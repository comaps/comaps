#include "drape_frontend/my_position_controller.hpp"

#include "drape_frontend/animation/arrow_animation.hpp"
#include "drape_frontend/animation/interpolators.hpp"
#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/animation_utils.hpp"
#include "drape_frontend/drape_notifier.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/angles.hpp"
#include "geometry/any_rect2d.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include "platform/location.hpp"
#include "platform/measurement_utils.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "routing/base/followed_polyline.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <vector>

namespace df
{
namespace
{
int constexpr kPositionRoutingOffsetY = 104;
// Speed threshold to switch to GPS bearing. Use compass for slow walking only.
double constexpr kMinSpeedThresholdMps = 0.7;  // 2.5 km/h
double constexpr kGpsBearingLifetimeSec = 5.0;
double constexpr kMaxTimeInBackgroundSec = 60.0 * 60 * 30;  // 30 hours before starting detecting position again
double constexpr kMaxNotFollowRoutingTimeSec = 20.0;
double constexpr kMaxUpdateLocationInvervalSec = 30.0;
double constexpr kMaxBlockAutoZoomTimeSec = 10.0;

int constexpr kZoomThreshold = 10;
int constexpr kMaxScaleZoomLevel = 16;
int constexpr kDefaultAutoZoom = 16;
double constexpr kUnknownAutoZoom = -1.0;

double constexpr kLookaheadTimeSpeedRatio = 2.1;
double constexpr kDefaultLookaheadSpeedMps = 14;  // 50 km/h
double constexpr kMaxLookaheadTimeSec = 40.0;
int constexpr kLookaheadSamples = 6;
int constexpr kExtendedSamplesThresholdM = 1000;
std::array<double, kLookaheadSamples> constexpr kLookaheadSampleMultipliers = {{0.15, 0.5, 1, 0.275, 0.35, 0.75}};

inline int GetZoomLevel(ScreenBase const & screen)
{
  return static_cast<int>(df::GetZoomLevel(screen.GetScale()));
}

int GetZoomLevel(ScreenBase const & screen, m2::PointD const & position, double errorRadius)
{
  ScreenBase s = screen;
  m2::PointD const size(errorRadius, errorRadius);
  s.SetFromRect(
      m2::AnyRectD(position, ang::Angle<double>(screen.GetAngle()), m2::RectD(position - size, position + size)));
  return GetZoomLevel(s);
}

inline double GetVisualScale()
{
  return df::VisualParams::Instance().GetVisualScale();
}

double CalculateZoomByMaxSpeed(double speedMpS, bool isPerspectiveAllowed)
{
  using TSpeedScale = std::pair<double, double>;
  static std::array<TSpeedScale, 2> const scales2d = {{
      {0.0, 1.75}, {77.0, 4.50}  // 48 mph
  }};
  static std::array<TSpeedScale, 2> const scales3d = {{{0.0, 1.00}, {77.0, 4.50}}};

  std::array<TSpeedScale, 2> const & scales = isPerspectiveAllowed ? scales3d : scales2d;

  double constexpr kDefaultSpeedLimitKmpH = 50.0;
  double const speedKmpH = speedMpS > 0 ? measurement_utils::MpsToKmph(speedMpS) : kDefaultSpeedLimitKmpH;

  size_t i = 0;
  for (size_t sz = scales.size(); i < sz; ++i)
    if (scales[i].first >= speedKmpH)
      break;

  double const vs = GetVisualScale();

  if (i == 0)
    return scales.front().second / vs;

  return scales[i - 1].second / vs;
}

double CalculateZoomByDistanceToTurn(double distance, bool isPerspectiveAllowed)
{
  using TSpeedScale = std::pair<double, double>;
  static std::array<TSpeedScale, 5> const scales2d = {{
      {200, 0.50},
      {500, 0.80},   // 0.3 mi
      {1200, 1.75},  // 0.75 mi
      {2000, 3.50},  // 1.2 mi
      {5000, 6.50}   // 3 mi
  }};
  static std::array<TSpeedScale, 5> const scales3d = {{
      {200, 0.25},
      {500, 0.60},   // 0.3 mi
      {1200, 1.25},  // 0.75 mi
      {2000, 3.50},  // 1.2 mi
      {5000, 6.50}   // 3 mi
  }};

  std::array<TSpeedScale, 5> const & scale = isPerspectiveAllowed ? scales3d : scales2d;

  double constexpr kDefaultDistance = 2000;
  double const distanceM = distance >= 0 ? distance : kDefaultDistance;

  size_t i = 0;
  for (size_t sz = scale.size(); i < sz; ++i)
    if (scale[i].first >= distanceM)
      break;

  double const vs = GetVisualScale();

  if (i == 0)
    return scale.front().second / vs;
  if (i == scale.size())
    return scale.back().second / vs;

  double const minDist = scale[i - 1].first;
  double const maxDist = scale[i].first;
  double const k = (distanceM - minDist) / (maxDist - minDist);

  double const minScale = scale[i - 1].second;
  double const maxScale = scale[i].second;
  double const zoom = minScale + k * (maxScale - minScale);

  return zoom / vs;
}

double CalculateAutoZoom(double speedMpS, double distanceToTurn, bool isPerspectiveAllowed)
{
  double const zoomByDistance = CalculateZoomByDistanceToTurn(distanceToTurn, isPerspectiveAllowed);
  double const zoomByMaxSpeed = CalculateZoomByMaxSpeed(speedMpS, isPerspectiveAllowed);

  return std::min(zoomByDistance, zoomByMaxSpeed);
}

void ResetNotification(uint64_t & notifyId)
{
  notifyId = DrapeNotifier::kInvalidId;
}

bool IsModeChangeViewport(location::EMyPositionMode mode)
{
  return mode == location::Follow || mode == location::FollowAndRotate;
}
}  // namespace

MyPositionController::MyPositionController(Params && params, ref_ptr<DrapeNotifier> notifier)
  : m_notifier(notifier)
  , m_modeChangeCallback(std::move(params.m_myPositionModeCallback))
  , m_hints(params.m_hints)
  , m_isInRouting(params.m_isRoutingActive)
  , m_needBlockAnimation(false)
  , m_wasRotationInScaling(false)
  , m_errorRadius(0.0)
  , m_horizontalAccuracy(0.0)
  , m_position(m2::PointD::Zero())
  , m_direction(0.0)
  , m_routeDirection(0.0)
  , m_arrowDirection(0.0)
  , m_oldPosition(m2::PointD::Zero())
  , m_oldArrowDirection(0.0)
  , m_enablePerspectiveInRouting(false)
  , m_enableAutoZoomInRouting(params.m_isAutozoomEnabled)
  , m_autoScale2d(GetScreenScale(kDefaultAutoZoom))
  , m_autoScale3d(m_autoScale2d)
  , m_lastGPSBearingTimer(false)
  , m_lastLocationTimestamp(0.0)
  , m_positionRoutingOffsetY(kPositionRoutingOffsetY * GetVisualScale())
  , m_isDirtyViewport(false)
  , m_isDirtyAutoZoom(false)
  , m_isPendingAnimation(false)
  , m_isPositionAssigned(false)
  , m_isArrowDirectionAssigned(false)
  , m_isRouteDirectionAssigned(false)
  , m_isCompassAvailable(false)
  , m_positionIsObsolete(false)
  , m_needBlockAutoZoom(false)
  , m_routingNotFollowNotifyId(DrapeNotifier::kInvalidId)
  , m_blockAutoZoomNotifyId(DrapeNotifier::kInvalidId)
  , m_updateLocationNotifyId(DrapeNotifier::kInvalidId)
{
  using namespace location;

  m_mode = PendingPosition;
  if (m_hints.m_isLaunchByDeepLink)
  {
    m_desiredInitMode = NotFollow;
  }
  else if (m_hints.m_isFirstLaunch)
  {
    m_desiredInitMode = Follow;
  }
  else if (params.m_timeInBackground >= kMaxTimeInBackgroundSec)
  {
    m_desiredInitMode = Follow;
  }
  else
  {
    m_desiredInitMode = params.m_initMode;

    // Do not start position if we ended previous session without it.
    if (!m_isInRouting && m_desiredInitMode == NotFollowNoPosition)
      m_mode = NotFollowNoPosition;
  }

  m_preferredRoutingMode = params.m_preferredRoutingMode;

  if (m_modeChangeCallback)
    m_modeChangeCallback(m_mode, m_isInRouting, false);
}

void MyPositionController::UpdatePosition()
{
  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::OnUpdateScreen(ScreenBase const & screen)
{
  m_pixelRect = screen.PixelRectIn3d();
  if (m_visiblePixelRect.IsEmptyInterior())
    SetVisibleViewport(m_pixelRect);
}

void MyPositionController::SetVisibleViewport(m2::RectD const & rect)
{
  m_visiblePixelRect = rect;
}

void MyPositionController::SetListener(ref_ptr<MyPositionController::Listener> listener)
{
  m_listener = listener;
}

m2::PointD const & MyPositionController::Position() const
{
  return m_position;
}

double MyPositionController::GetErrorRadius() const
{
  return m_errorRadius;
}

double MyPositionController::GetHorizontalAccuracy() const
{
  return m_horizontalAccuracy;
}

bool MyPositionController::IsModeChangeViewport() const
{
  return df::IsModeChangeViewport(m_mode);
}

bool MyPositionController::IsModeHasPosition() const
{
  return m_mode != location::PendingPosition && m_mode != location::NotFollowNoPosition;
}

void MyPositionController::DragStarted()
{
  m_needBlockAnimation = true;
}

void MyPositionController::DragEnded(m2::PointD const & distance)
{
  float constexpr kBindingDistance = 0.1f;
  m_needBlockAnimation = false;
  if (distance.Length() > kBindingDistance * std::min(m_pixelRect.SizeX(), m_pixelRect.SizeY()))
    StopLocationFollow();

  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::ScaleStarted()
{
  m_needBlockAnimation = true;
  ResetBlockAutoZoomTimer();
}

void MyPositionController::ScaleEnded()
{
  m_needBlockAnimation = false;
  ResetBlockAutoZoomTimer();
  if (m_wasRotationInScaling)
  {
    m_wasRotationInScaling = false;
    StopLocationFollow();
  }

  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::Rotated()
{
  if (m_mode == location::FollowAndRotate)
    m_wasRotationInScaling = true;
}

void MyPositionController::Scrolled(m2::PointD const & distance)
{
  if (m_mode == location::PendingPosition)
    return;

  if (distance.Length() > 0)
    StopLocationFollow();

  UpdateViewport(kDoNotChangeZoom);
}

void MyPositionController::ResetRoutingNotFollowTimer(bool blockTimer)
{
  if (m_isInRouting)
  {
    m_routingNotFollowTimer.Reset();
    m_blockRoutingNotFollowTimer = blockTimer;
    ResetNotification(m_routingNotFollowNotifyId);
  }
}

void MyPositionController::ResetBlockAutoZoomTimer()
{
  if (m_isInRouting && m_enableAutoZoomInRouting)
  {
    m_needBlockAutoZoom = true;
    m_blockAutoZoomTimer.Reset();
    ResetNotification(m_blockAutoZoomNotifyId);
  }
}

void MyPositionController::CorrectScalePoint(m2::PointD & pt) const
{
  if (IsModeChangeViewport())
    pt = GetRotationPixelCenter();
}

void MyPositionController::CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const
{
  if (IsModeChangeViewport())
  {
    m2::PointD const oldPt1(pt1);
    pt1 = GetRotationPixelCenter();
    pt2 = pt2 - oldPt1 + pt1;
  }
}

void MyPositionController::CorrectGlobalScalePoint(m2::PointD & pt) const
{
  if (IsModeChangeViewport())
    pt = m_position;
}

void MyPositionController::SetRenderShape(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> texMng,
                                          drape_ptr<MyPosition> && shape, Arrow3d::PreloadedData && preloadedData)
{
  m_shape = std::move(shape);
  if (!m_shape->InitArrow(context, texMng, std::move(preloadedData)))
  {
    m_shape.reset();
    LOG(LERROR, ("Invalid Arrow3D mesh."));
  }
}

void MyPositionController::ResetRenderShape()
{
  m_shape.reset();
}

void MyPositionController::NextMode(ScreenBase const & screen)
{
  // When the app is awaiting location (indicator is active) and the user presses on the indicator, location updates
  // will be stopped and goes into NotFollowNoPosition state. The next press on the indicator will start location
  // updates again.
  if (IsWaitingForLocation())
  {
    m_desiredInitMode = location::Follow;
    ChangeMode(location::NotFollowNoPosition);
    return;
  }

  // Start looking for location.
  if (m_mode == location::NotFollowNoPosition)
  {
    ChangeMode(location::PendingPosition);

    if (!m_isPositionAssigned)
    {
      // This is the first user location request (button touch) after controller's initialization
      // with some previous not Follow state. The new mode will be Follow to center on the position.
      m_desiredInitMode = location::Follow;
    }
    return;
  }

  // Calculate preferred zoom level.
  int const currentZoom = GetZoomLevel(screen);
  int preferredZoomLevel = kDoNotChangeZoom;
  if (currentZoom < kZoomThreshold)
    preferredZoomLevel = std::min(GetZoomLevel(screen, m_position, m_errorRadius), kMaxScaleZoomLevel);

  // In routing not-follow -> follow-and-rotate, otherwise not-follow -> follow.
  if (m_mode == location::NotFollow)
  {
    if (m_lastFollowMode == location::FollowAndRotate &&
        (IsArrowRotationAvailable() || (m_preferRouteDirectionInRouting && IsRouteRotationAvailable())))
      ChangeMode(location::FollowAndRotate, true);
    else
      ChangeMode(location::Follow, true);

    UpdateViewport(preferredZoomLevel);
    return;
  }

  // From follow mode we transit to follow-and-rotate if compass is available or
  // routing is enabled.
  if (m_mode == location::Follow)
  {
    if (IsArrowRotationAvailable() || m_isInRouting)
    {
      ChangeMode(location::FollowAndRotate, true);
      UpdateViewport(preferredZoomLevel);
    }
    return;
  }

  // From follow-and-rotate mode we can transit to follow mode.
  if (m_mode == location::FollowAndRotate)
  {
    if (m_isInRouting && screen.isPerspective())
      preferredZoomLevel = static_cast<int>(GetZoomLevel(ScreenBase::GetStartPerspectiveScale() * 1.1));
    ChangeMode(location::Follow, true);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), preferredZoomLevel);
  }
}

void MyPositionController::StartPendingPositionMode()
{
  if (m_mode == location::NotFollowNoPosition)
  {
    ChangeMode(location::PendingPosition);
    return;
  }
}

void MyPositionController::OnLocationUpdate(location::GpsInfo const & info,
                                            df::NavigationContext const & navigationContext, ScreenBase const & screen)
{
  m2::PointD const oldPos = GetDrawablePosition();
  double const oldAzimut = GetDrawableAzimut();

  m2::RectD const rect = mercator::MetersToXY(info.m_longitude, info.m_latitude, info.m_horizontalAccuracy);
  // Use FromLatLon instead of rect.Center() since in case of large info.m_horizontalAccuracy
  // there is significant difference between the real location and the estimated one.
  m_position = mercator::FromLatLon(info.m_latitude, info.m_longitude);
  m_errorRadius = rect.SizeX() * 0.5;
  m_horizontalAccuracy = info.m_horizontalAccuracy;

  if (navigationContext.m_distanceToNextTurn >= 0.0 || navigationContext.m_speedLimit >= 0.0)
  {
    double const mercatorPerMeter = m_errorRadius / info.m_horizontalAccuracy;
    m_autoScale2d =
        mercatorPerMeter * CalculateAutoZoom(navigationContext.m_speedLimit, navigationContext.m_distanceToNextTurn,
                                             false /* isPerspectiveAllowed */);
    m_autoScale3d =
        mercatorPerMeter * CalculateAutoZoom(navigationContext.m_speedLimit, navigationContext.m_distanceToNextTurn,
                                             true /* isPerspectiveAllowed */);
  }
  else
  {
    m_autoScale2d = m_autoScale3d = kUnknownAutoZoom;
  }

  // Sets arrow direction based on GPS if:
  // 1. Compass is not available.
  // 2. Direction must be glued to the route during routing (route-corrected angle is set only in
  // OnLocationUpdate(): in OnCompassUpdate() the angle always has the original value.
  // 3. Device is moving faster then pedestrian.
  bool const isMovingFast = info.HasSpeed() && info.m_speed > kMinSpeedThresholdMps;
  bool const glueArrowInRouting = navigationContext.m_isNavigable && m_isArrowGluedInRouting;

  // Calculate the route direction by sampling points along the route up to the lookahead distance
  // and taking the average direction
  if (glueArrowInRouting && navigationContext.m_followedPolyline != nullptr)
  {
    double const speedLimitMps =
        (navigationContext.m_speedLimit > 0 ? navigationContext.m_speedLimit : kDefaultLookaheadSpeedMps);
    double const lookaheadTimeSec = std::min(speedLimitMps * kLookaheadTimeSpeedRatio, kMaxLookaheadTimeSec);
    double const lookaheadDistanceM =
        std::min(speedLimitMps * lookaheadTimeSec, navigationContext.m_distanceToNextTurn);

    m2::PointD const currentPoint = navigationContext.m_followedPolyline->GetCurrentIter().m_pt;

    int const numSamples =
        navigationContext.m_distanceToNextTurn < kExtendedSamplesThresholdM ? kLookaheadSamples / 2 : kLookaheadSamples;

    ang::AverageCalc calc;
    bool valid = true;

    for (int i = 0; i < numSamples; i++)
    {
      double const sampleDist = kLookaheadSampleMultipliers[i] * lookaheadDistanceM;
      m2::PointD const samplePoint = navigationContext.m_followedPolyline->GetLookaheadPoint(sampleDist);
      double const angle = ang::AngleTo(currentPoint, samplePoint);

      if (std::isnan(angle) || std::isinf(angle))
      {
        valid = false;
        break;
      }

      calc.Add(angle);
    }

    if (!valid)
    {
      SetRouteDirection(info.m_bearing);
    }
    else
    {
      // The angle is clamped to a cone in front of the driver to prevent situations
      // where the route direction faces the opposite way the driver is actually
      // going
      double const bisect = ang::AngleTo(currentPoint, navigationContext.m_followedPolyline->GetLookaheadPoint(
                                                           std::min(speedLimitMps, lookaheadDistanceM * 0.1)));
      SetRouteDirection(location::RadiansToBearing(ang::ClampAngle(calc.GetAverage(), bisect, math::DegToRad(15.0))));
    }
  }

  if ((!m_isCompassAvailable || glueArrowInRouting || isMovingFast) && info.HasBearing())
  {
    SetArrowDirection(math::DegToRad(info.m_bearing));
    m_lastGPSBearingTimer.Reset();
  }

  m_direction = m_preferRouteDirectionInRouting ? m_routeDirection : m_arrowDirection;

  if (m_isPositionAssigned && (!AlmostCurrentPosition(oldPos) || !AlmostCurrentAzimut(oldAzimut)))
  {
    CreateAnim(oldPos, oldAzimut, screen);
    m_isDirtyViewport = true;
  }

  // Assume that every new position is fresh enough. We can't make some straightforward filtering here
  // like comparing system_clock::now().time_since_epoch() and info.m_timestamp, because can't rely
  // on valid time settings on endpoint device.
  m_positionIsObsolete = false;

  if (!m_isPositionAssigned)
  {
    ChangeMode(m_isInRouting ? m_preferredRoutingMode : m_desiredInitMode, true);

    if (!m_hints.m_isFirstLaunch || !AnimationSystem::Instance().AnimationExists(Animation::Object::MapPlane))
    {
      if (m_mode == location::Follow)
      {
        ChangeModelView(m_position, kDoNotChangeZoom);
      }
      else if (m_mode == location::FollowAndRotate)
      {
        ChangeModelView(m_position, m_direction,
                        m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center(),
                        kDoNotChangeZoom);
      }
    }
  }
  else if (m_mode == location::PendingPosition)
  {
    if (m_isInRouting && m_preferredRoutingMode == location::FollowAndRotate)
    {
      ChangeMode(location::FollowAndRotate, true);
      UpdateViewport(kMaxScaleZoomLevel);
    }
    else
    {
      ChangeMode(location::Follow, true);
      if (m_hints.m_isFirstLaunch)
      {
        if (!AnimationSystem::Instance().AnimationExists(Animation::Object::MapPlane))
          ChangeModelView(m_position, kDoNotChangeZoom);
      }
      else if (GetZoomLevel(screen, m_position, m_errorRadius) <= kMaxScaleZoomLevel)
      {
        m2::PointD const size(m_errorRadius, m_errorRadius);
        ChangeModelView(m2::RectD(m_position - size, m_position + size));
      }
      else
      {
        ChangeModelView(m_position, kMaxScaleZoomLevel);
      }
    }
  }
  else if (m_mode == location::NotFollowNoPosition)
  {
    if (m_isInRouting && m_preferredRoutingMode == location::FollowAndRotate)
    {
      ChangeMode(location::FollowAndRotate, true);
      UpdateViewport(kMaxScaleZoomLevel);
    }
    else
    {
      // Here we silently get the position and go to NotFollow mode.
      ChangeMode(location::NotFollow, true);
    }
  }

  m_isPositionAssigned = true;

  if (m_listener != nullptr)
    m_listener->PositionChanged(Position(), IsModeHasPosition());

  if (fabs(m_lastLocationTimestamp - info.m_timestamp) > 1.0E-5)
  {
    m_lastLocationTimestamp = info.m_timestamp;
    m_updateLocationTimer.Reset();
    ResetNotification(m_updateLocationNotifyId);
  }
}

void MyPositionController::LoseLocation()
{
  if (m_mode == location::NotFollowNoPosition)
    return;
  else if (m_mode == location::Follow || m_mode == location::FollowAndRotate)
    ChangeMode(location::PendingPosition);
  else
    ChangeMode(location::NotFollowNoPosition);

  if (m_listener != nullptr)
    m_listener->PositionChanged(Position(), false /* hasPosition */);
}

void MyPositionController::OnCompassUpdate(location::CompassInfo const & info, ScreenBase const & screen)
{
  double const oldAzimut = GetDrawableAzimut();
  m_isCompassAvailable = true;

  bool const existsFreshGpsBearing = m_lastGPSBearingTimer.ElapsedSeconds() < kGpsBearingLifetimeSec;
  if ((IsInRouting() && m_isArrowGluedInRouting) || existsFreshGpsBearing)
    return;

  SetArrowDirection(info.m_bearing);

  if (m_isPositionAssigned && !AlmostCurrentAzimut(oldAzimut) && m_mode == location::FollowAndRotate &&
      !m_preferRouteDirectionInRouting)
  {
    m_direction = info.m_bearing;
    CreateAnim(GetDrawablePosition(), oldAzimut, screen);
    m_isDirtyViewport = true;
  }
}

bool MyPositionController::UpdateViewportWithAutoZoom()
{
  double const autoScale = m_enablePerspectiveInRouting ? m_autoScale3d : m_autoScale2d;
  if (autoScale > 0.0 && m_mode == location::FollowAndRotate && m_isInRouting && m_enableAutoZoomInRouting &&
      !m_needBlockAutoZoom)
  {
    ChangeModelView(autoScale, m_position, m_direction, GetRoutingRotationPixelCenter());
    return true;
  }
  return false;
}

void MyPositionController::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                  ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  CheckNotFollowRouting();

  if (m_shape != nullptr && IsModeHasPosition())
  {
    CheckBlockAutoZoom();
    CheckUpdateLocation();

    if ((m_isDirtyViewport || m_isDirtyAutoZoom) && !m_needBlockAnimation)
    {
      if (!UpdateViewportWithAutoZoom() && m_isDirtyViewport)
        UpdateViewport(kDoNotChangeZoom);
      m_isDirtyViewport = false;
      m_isDirtyAutoZoom = false;
    }

    if (!IsModeChangeViewport())
      m_isPendingAnimation = false;

    /// @todo Put under !m_hints.m_screenshotMode?
    /// Why do we have 6 modifiers (and 6 variables inside), if better to make 1 function m_shape->Render(Params)?
    m_shape->SetPositionObsolete(m_positionIsObsolete);
    m_shape->SetPosition(m2::PointF(GetDrawablePosition()));
    m_shape->SetAzimuth(static_cast<float>(GetDrawableAzimut()));
    m_shape->SetIsValidAzimuth(IsArrowRotationAvailable());
    m_shape->SetAccuracy(static_cast<float>(m_errorRadius));
    m_shape->SetRoutingMode(IsInRouting());

    if (!m_hints.m_screenshotMode)
    {
      m_shape->RenderAccuracy(context, mng, screen, zoomLevel, frameValues);
      m_shape->RenderMyPosition(context, mng, screen, zoomLevel, frameValues);
    }
  }
}

bool MyPositionController::IsRouteFollowingActive() const
{
  return IsInRouting() && m_mode == location::FollowAndRotate;
}

bool MyPositionController::AlmostCurrentPosition(m2::PointD const & pos) const
{
  double constexpr kPositionEqualityDelta = 1e-5;
  return pos.EqualDxDy(m_position, kPositionEqualityDelta);
}

bool MyPositionController::AlmostCurrentAzimut(double azimut) const
{
  double constexpr kDirectionEqualityDelta = 1e-3;
  return AlmostEqualAbs(azimut, m_direction, kDirectionEqualityDelta);
}

void MyPositionController::SetRouteDirection(double bearing)
{
  m_routeDirection = bearing;
  m_isRouteDirectionAssigned = true;
}

void MyPositionController::SetArrowDirection(double bearing)
{
  m_arrowDirection = bearing;
  m_isArrowDirectionAssigned = true;
}

void MyPositionController::ChangeMode(location::EMyPositionMode newMode)
{
  ChangeMode(newMode, false);
}

void MyPositionController::ChangeMode(location::EMyPositionMode newMode, bool shouldPersist)
{
  if (m_isInRouting && (m_mode != newMode) && (newMode == location::FollowAndRotate))
    ResetBlockAutoZoomTimer();

  m_mode = newMode;

  // We should remember this mode if the user intentionally caused the mode change
  // (e.g. tapping the mode button)
  if (m_isInRouting && shouldPersist)
    m_preferredRoutingMode = newMode;

  if (m_modeChangeCallback)
    m_modeChangeCallback(m_mode, m_isInRouting, shouldPersist);
}

bool MyPositionController::IsWaitingForLocation() const
{
  if (m_mode == location::NotFollowNoPosition)
    return false;

  if (!m_isPositionAssigned)
    return true;

  return m_mode == location::PendingPosition;
}

void MyPositionController::StopLocationFollow()
{
  if (m_mode == location::Follow || m_mode == location::FollowAndRotate)
  {
    m_lastFollowMode = m_mode;
    ChangeMode(location::NotFollow);
  }
  m_desiredInitMode = location::NotFollow;

  ResetRoutingNotFollowTimer();
}

void MyPositionController::OnEnterForeground(double backgroundTime)
{
  // Handle the case when the app was in the background for a long time and the user is opening the app.
  if (backgroundTime >= kMaxTimeInBackgroundSec)
  {
    // When location was active during previous session the app will try to follow the user.
    if (m_mode == location::NotFollow)
    {
      ChangeMode((m_isInRouting && m_preferredRoutingMode == location::FollowAndRotate) ? location::FollowAndRotate
                                                                                        : location::Follow);
      UpdateViewport(kDoNotChangeZoom);
    }

    // When location was stopped by the user manually app will try to find position but without following.
    else if (m_mode == location::NotFollowNoPosition)
    {
      ChangeMode(location::PendingPosition);
    }
  }
}

void MyPositionController::OnEnterBackground() {}

void MyPositionController::OnCompassTapped()
{
  if (m_mode == location::FollowAndRotate)
  {
    ChangeMode(location::Follow);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), kDoNotChangeZoom);
  }
  else
  {
    ChangeModelView(0.0);
  }
}

void MyPositionController::ChangeModelView(m2::PointD const & center, int zoomLevel)
{
  if (m_listener)
    m_listener->ChangeModelView(center, zoomLevel, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(double azimuth)
{
  if (m_listener)
    m_listener->ChangeModelView(azimuth, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(m2::RectD const & rect)
{
  if (m_listener)
    m_listener->ChangeModelView(rect, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pxZero,
                                           int zoomLevel, Animation::TAction const & onFinishAction)
{
  if (m_listener)
    m_listener->ChangeModelView(userPos, azimuth, pxZero, zoomLevel, onFinishAction, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::ChangeModelView(double autoScale, m2::PointD const & userPos, double azimuth,
                                           m2::PointD const & pxZero)
{
  if (m_listener)
    m_listener->ChangeModelView(autoScale, userPos, azimuth, pxZero, m_animCreator);
  m_animCreator = nullptr;
}

void MyPositionController::UpdateViewport(int zoomLevel)
{
  if (IsWaitingForLocation())
    return;

  if (m_mode == location::Follow)
  {
    ChangeModelView(m_position, zoomLevel);
  }
  else if (m_mode == location::FollowAndRotate)
  {
    ChangeModelView(m_position, m_direction,
                    m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center(), zoomLevel);
  }
}

m2::PointD MyPositionController::GetRotationPixelCenter() const
{
  if (m_mode == location::Follow)
    return m_visiblePixelRect.Center();

  if (m_mode == location::FollowAndRotate)
    return m_isInRouting ? GetRoutingRotationPixelCenter() : m_visiblePixelRect.Center();

  return m2::PointD::Zero();
}

m2::PointD MyPositionController::GetRoutingRotationPixelCenter() const
{
  return {m_visiblePixelRect.Center().x, m_visiblePixelRect.maxY() - m_positionRoutingOffsetY};
}

void MyPositionController::UpdateRoutingOffsetY(bool useDefault, int offsetY)
{
  double const vs = GetVisualScale();
  m_positionRoutingOffsetY = useDefault ? kPositionRoutingOffsetY * vs : offsetY + Arrow3d::GetMaxBottomSize() * vs;
}

m2::PointD MyPositionController::GetDrawablePosition()
{
  m2::PointD position;
  if (AnimationSystem::Instance().GetArrowPosition(position))
  {
    m_isPendingAnimation = false;
    return position;
  }

  if (m_isPendingAnimation)
    return m_oldPosition;

  return m_position;
}

double MyPositionController::GetDrawableAzimut()
{
  double angle;
  if (AnimationSystem::Instance().GetArrowAngle(angle))
  {
    m_isPendingAnimation = false;
    return angle;
  }

  if (m_isPendingAnimation)
    return m_oldArrowDirection;

  return m_arrowDirection;
}

void MyPositionController::CreateAnim(m2::PointD const & oldPos, double oldAzimut, ScreenBase const & screen)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(oldPos, m_position, screen);
  double const rotateDuration = AngleInterpolator::GetRotateDuration(oldAzimut, m_arrowDirection);
  if (df::IsAnimationAllowed(std::max(moveDuration, rotateDuration), screen))
  {
    if (IsModeChangeViewport())
    {
      m_animCreator = [this, moveDuration](ref_ptr<Animation> syncAnim) -> drape_ptr<Animation>
      {
        drape_ptr<Animation> anim = make_unique_dp<ArrowAnimation>(
            GetDrawablePosition(), m_position, syncAnim == nullptr ? moveDuration : syncAnim->GetDuration(),
            GetDrawableAzimut(), m_arrowDirection);
        if (syncAnim != nullptr)
        {
          anim->SetMaxDuration(syncAnim->GetMaxDuration());
          anim->SetMinDuration(syncAnim->GetMinDuration());
        }
        return anim;
      };
      m_oldPosition = oldPos;
      m_oldArrowDirection = oldAzimut;
      m_isPendingAnimation = true;
    }
    else
    {
      AnimationSystem::Instance().CombineAnimation(
          make_unique_dp<ArrowAnimation>(oldPos, m_position, moveDuration, oldAzimut, m_arrowDirection));
    }
  }
}

void MyPositionController::EnablePerspectiveInRouting(bool enablePerspective)
{
  m_enablePerspectiveInRouting = enablePerspective;
}

void MyPositionController::EnableAutoZoomInRouting(bool enableAutoZoom)
{
  if (m_isInRouting)
  {
    m_enableAutoZoomInRouting = enableAutoZoom;
    ResetBlockAutoZoomTimer();
  }
}

void MyPositionController::ActivateRouting(int zoomLevel, bool enableAutoZoom, bool isArrowGlued,
                                           bool allowRouteRotation)
{
  if (!m_isInRouting)
  {
    m_isInRouting = true;
    m_isArrowGluedInRouting = isArrowGlued;
    m_enableAutoZoomInRouting = enableAutoZoom;
    m_preferRouteDirectionInRouting = allowRouteRotation;

    if (m_preferredRoutingMode == location::Follow)
    {
      ChangeMode(location::Follow);
      ChangeModelView(m_position, kDoNotChangeZoom);
    }
    else
    {
      ChangeMode(location::FollowAndRotate);
      ChangeModelView(m_position, m_isRouteDirectionAssigned ? m_routeDirection : m_arrowDirection,
                      GetRoutingRotationPixelCenter(), zoomLevel,
                      [this](ref_ptr<Animation> anim) { UpdateViewport(kDoNotChangeZoom); });
    }
    ResetRoutingNotFollowTimer();
  }
}

void MyPositionController::DeactivateRouting()
{
  if (m_isInRouting)
  {
    m_isInRouting = false;
    m_isArrowGluedInRouting = false;
    m_preferRouteDirectionInRouting = false;

    m_isArrowDirectionAssigned = m_isCompassAvailable && m_isArrowDirectionAssigned;
    m_isRouteDirectionAssigned = false;

    ChangeMode(location::Follow);
    ChangeModelView(m_position, 0.0, m_visiblePixelRect.Center(), kDoNotChangeZoom);
  }
}

// This code schedules the execution of checkFunction on FR after timeout. Additionally
// there is the protection from multiple scheduling.
#define CHECK_ON_TIMEOUT(id, timeout, checkFunction)                                                               \
  if (id == DrapeNotifier::kInvalidId)                                                                             \
  {                                                                                                                \
    id = m_notifier->Notify(ThreadsCommutator::RenderThread, std::chrono::seconds(static_cast<uint32_t>(timeout)), \
                            false /* repeating */, [this](uint64_t notifyId)                                       \
    {                                                                                                              \
      if (notifyId != id)                                                                                          \
        return;                                                                                                    \
      checkFunction();                                                                                             \
      id = DrapeNotifier::kInvalidId;                                                                              \
    });                                                                                                            \
  }

void MyPositionController::CheckNotFollowRouting()
{
  if (!m_blockRoutingNotFollowTimer && IsInRouting() && m_mode == location::NotFollow)
  {
    CHECK_ON_TIMEOUT(m_routingNotFollowNotifyId, kMaxNotFollowRoutingTimeSec, CheckNotFollowRouting);
    if (m_routingNotFollowTimer.ElapsedSeconds() >= kMaxNotFollowRoutingTimeSec)
    {
      ChangeMode(m_preferredRoutingMode);
      UpdateViewport(kDoNotChangeZoom);
    }
  }
}

void MyPositionController::CheckBlockAutoZoom()
{
  if (m_needBlockAutoZoom)
  {
    CHECK_ON_TIMEOUT(m_blockAutoZoomNotifyId, kMaxBlockAutoZoomTimeSec, CheckBlockAutoZoom);
    if (m_blockAutoZoomTimer.ElapsedSeconds() >= kMaxBlockAutoZoomTimeSec)
    {
      m_needBlockAutoZoom = false;
      m_isDirtyAutoZoom = true;
    }
  }
}

void MyPositionController::CheckUpdateLocation()
{
  if (!m_positionIsObsolete)
  {
    CHECK_ON_TIMEOUT(m_updateLocationNotifyId, kMaxUpdateLocationInvervalSec, CheckUpdateLocation);
    if (m_updateLocationTimer.ElapsedSeconds() >= kMaxUpdateLocationInvervalSec)
    {
      m_positionIsObsolete = true;
      m_autoScale2d = m_autoScale3d = kUnknownAutoZoom;
    }
  }
}

#undef CHECK_ON_TIMEOUT
}  // namespace df
