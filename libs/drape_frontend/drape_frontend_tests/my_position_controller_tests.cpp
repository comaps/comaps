#include "testing/testing.hpp"

#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/screenbase.hpp"

#include "base/math.hpp"

#include <utility>
#include <vector>

namespace my_position_controller_tests
{
namespace
{
double constexpr kBearingDegrees = 45.0;
double constexpr kEpsilon = 1e-9;
int constexpr kRoutingZoom = 16;
m2::RectD const kVisibleViewport(100.0, 200.0, 900.0, 800.0);

struct ModeChange
{
  location::EMyPositionMode m_mode;
  bool m_routingActive;
  bool m_shouldPersist;
};

using ModeChanges = std::vector<ModeChange>;

ScreenBase MakeScreen()
{
  ScreenBase screen;
  screen.SetFromRects(m2::AnyRectD(m2::RectD(0.0, 0.0, 100.0, 100.0)), m2::RectD(0.0, 0.0, 1000.0, 1000.0));
  return screen;
}

location::GpsInfo MakeGpsInfo(bool withBearing)
{
  location::GpsInfo info;
  info.m_source = location::EUser;
  info.m_timestamp = 1.0;
  info.m_latitude = 10.0;
  info.m_longitude = 20.0;
  info.m_horizontalAccuracy = 5.0;
  if (withBearing)
  {
    info.m_bearing = kBearingDegrees;
    info.m_speed = 5.0;
  }
  return info;
}

df::NavigationContext MakeRoutingContext()
{
  return {true /* navigable */,
          500.0 /* distanceToTurn */,
          10.0 /* speedLimit */,
          m2::PointD(0.0, 0.0) /* routePoint */,
          {m2::PointD(0.0, 0.0), m2::PointD(0.0, 1000.0)} /* polylineSubset */};
}

df::MyPositionController::Params MakeParams(location::EMyPositionMode preferredRoutingMode, ModeChanges & changes)
{
  df::VisualParams::Init(1.0, 1024);
  return {location::Follow,
          preferredRoutingMode,
          0.0 /* timeInBackground */,
          df::Hints(),
          false /* isRoutingActive */,
          false /* isAutozoomEnabled */,
          [&changes](location::EMyPositionMode mode, bool routingActive, bool shouldPersist)
  { changes.push_back({mode, routingActive, shouldPersist}); }};
}

class RecordingListener : public df::MyPositionController::Listener
{
public:
  void PositionChanged(m2::PointD const & /* position */, bool /* hasPosition */) override {}

  void ChangeModelView(m2::PointD const & /* center */, int /* zoomLevel */,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {}

  void ChangeModelView(double /* azimuth */, df::TAnimationCreator const & /* parallelAnimCreator */) override {}

  void ChangeModelView(m2::RectD const & /* rect */, df::TAnimationCreator const & /* parallelAnimCreator */) override
  {}

  void ChangeModelView(m2::PointD const & userPos, double azimuth, m2::PointD const & pixelZero, int zoomLevel,
                       df::Animation::TAction const & /* onFinishAction */,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {
    m_hasFollowRequest = true;
    m_userPos = userPos;
    m_azimuth = azimuth;
    m_pixelZero = pixelZero;
    m_zoomLevel = zoomLevel;
  }

  void ChangeModelView(double /* autoScale */, m2::PointD const & /* userPos */, double /* azimuth */,
                       m2::PointD const & /* pixelZero */,
                       df::TAnimationCreator const & /* parallelAnimCreator */) override
  {}

  void Reset() { m_hasFollowRequest = false; }

  bool m_hasFollowRequest = false;
  m2::PointD m_userPos;
  double m_azimuth = 0.0;
  m2::PointD m_pixelZero;
  int m_zoomLevel = 0;
};

class ControllerFixture
{
public:
  ControllerFixture(location::EMyPositionMode preferredRoutingMode, bool withBearing)
    : m_controller(MakeParams(preferredRoutingMode, m_modeChanges), ref_ptr<df::DrapeNotifier>())
  {
    m_controller.SetListener(ref_ptr<df::MyPositionController::Listener>(&m_listener));
    m_controller.OnUpdateScreen(m_screen);
    m_controller.SetVisibleViewport(kVisibleViewport);
    m_controller.OnLocationUpdate(MakeGpsInfo(withBearing), df::NavigationContext(), m_screen);
    m_controller.ActivateRouting(kRoutingZoom, false /* enableAutoZoom */, true /* isArrowGlued */,
                                 true /* allowRouteRotation */);
  }

  void PrepareForDeactivation(bool withBearing)
  {
    m_controller.OnLocationUpdate(MakeGpsInfo(withBearing), MakeRoutingContext(), m_screen);
    m_listener.Reset();
    m_modeChanges.clear();
  }

  ModeChanges m_modeChanges;
  ScreenBase m_screen = MakeScreen();
  RecordingListener m_listener;
  df::MyPositionController m_controller;
};

void TestPostRoutingModeChange(ModeChanges const & changes, location::EMyPositionMode expectedMode)
{
  TEST_EQUAL(changes.size(), 1, ());
  TEST_EQUAL(changes.front().m_mode, expectedMode, ());
  TEST(!changes.front().m_routingActive, ());
  TEST(!changes.front().m_shouldPersist, ());
}
}  // namespace

UNIT_TEST(MyPositionController_DeactivateRoutingRestoresHeadingUp)
{
  ControllerFixture fixture(location::FollowAndRotate, true /* withBearing */);
  fixture.PrepareForDeactivation(true /* withBearing */);

  fixture.m_controller.DeactivateRouting();

  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(fixture.m_listener.m_hasFollowRequest, ());
  TEST_EQUAL(fixture.m_listener.m_userPos, fixture.m_controller.Position(), ());
  TEST_ALMOST_EQUAL_ABS(fixture.m_listener.m_azimuth, math::DegToRad(kBearingDegrees), kEpsilon, ());
  TEST_EQUAL(fixture.m_listener.m_pixelZero, kVisibleViewport.Center(), ());
  TEST_EQUAL(fixture.m_listener.m_zoomLevel, df::kDoNotChangeZoom, ());
  TestPostRoutingModeChange(fixture.m_modeChanges, location::FollowAndRotate);

  // Deactivation must not overwrite the persisted routing preference.
  fixture.m_controller.ActivateRouting(kRoutingZoom, false /* enableAutoZoom */, true /* isArrowGlued */,
                                       true /* allowRouteRotation */);
  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_DeactivateRoutingRestoresNorthUp)
{
  ControllerFixture fixture(location::Follow, true /* withBearing */);
  fixture.PrepareForDeactivation(true /* withBearing */);

  fixture.m_controller.DeactivateRouting();

  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::Follow, ());
  TEST(fixture.m_listener.m_hasFollowRequest, ());
  TEST_EQUAL(fixture.m_listener.m_userPos, fixture.m_controller.Position(), ());
  TEST_ALMOST_EQUAL_ABS(fixture.m_listener.m_azimuth, 0.0, kEpsilon, ());
  TEST_EQUAL(fixture.m_listener.m_pixelZero, kVisibleViewport.Center(), ());
  TEST_EQUAL(fixture.m_listener.m_zoomLevel, df::kDoNotChangeZoom, ());
  TestPostRoutingModeChange(fixture.m_modeChanges, location::Follow);

  fixture.m_controller.ActivateRouting(kRoutingZoom, false /* enableAutoZoom */, true /* isArrowGlued */,
                                       true /* allowRouteRotation */);
  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_DeactivateRoutingKeepsHeadingUpWithoutDirection)
{
  ControllerFixture fixture(location::FollowAndRotate, false /* withBearing */);
  fixture.PrepareForDeactivation(false /* withBearing */);

  fixture.m_controller.DeactivateRouting();

  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(!fixture.m_controller.IsArrowRotationAvailable(), ());
  TEST_ALMOST_EQUAL_ABS(fixture.m_listener.m_azimuth, 0.0, kEpsilon, ());
  TestPostRoutingModeChange(fixture.m_modeChanges, location::FollowAndRotate);

  fixture.m_modeChanges.clear();
  location::CompassInfo compassInfo;
  compassInfo.m_bearing = 1.0;
  fixture.m_controller.OnCompassUpdate(compassInfo, fixture.m_screen);

  TEST(fixture.m_controller.IsArrowRotationAvailable(), ());
  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST(fixture.m_modeChanges.empty(), ());
}

UNIT_TEST(MyPositionController_DeactivateRoutingNormalizesUnexpectedPreferenceToHeadingUp)
{
  ControllerFixture fixture(location::PendingPosition, true /* withBearing */);
  fixture.PrepareForDeactivation(true /* withBearing */);

  fixture.m_controller.DeactivateRouting();

  TEST_EQUAL(fixture.m_controller.GetCurrentMode(), location::FollowAndRotate, ());
  TEST_ALMOST_EQUAL_ABS(fixture.m_listener.m_azimuth, math::DegToRad(kBearingDegrees), kEpsilon, ());
  TestPostRoutingModeChange(fixture.m_modeChanges, location::FollowAndRotate);
}
}  // namespace my_position_controller_tests
