#include "testing/testing.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/screenbase.hpp"

namespace
{
double constexpr kLongBackgroundSec = 60.0 * 16;

class PositionFixture : public df::MyPositionController::Listener
{
public:
  PositionFixture(location::EMyPositionMode initMode, location::EMyPositionMode lastFollowMode,
                  double timeInBackground)
  {
    df::VisualParams::Init(1.0, 1024);
    m_controller = make_unique_dp<df::MyPositionController>(
        df::MyPositionController::Params(initMode, lastFollowMode, location::FollowAndRotate, timeInBackground,
                                         df::Hints{}, false /* isRoutingActive */, false /* isAutozoomEnabled */,
                                         [this](location::EMyPositionMode, bool, bool shouldPersist)
    { m_lastShouldPersist = shouldPersist; }),
        nullptr /* notifier */);
    m_controller->SetListener(make_ref(this));
    m_screen.SetFromParams(m2::PointD::Zero(), 0.0, df::GetScreenScale(15));
    m_controller->OnUpdateScreen(m_screen);
    m_context.m_distanceToNextTurn = -1.0;
    m_context.m_speedLimit = -1.0;
    m_gps.m_horizontalAccuracy = 5.0;
    m_gps.m_bearing = 90.0;
  }

  ~PositionFixture() override
  {
    df::AnimationSystem::Instance().FinishObjectAnimations(df::Animation::Object::MyPositionArrow, false, true);
  }

  void UpdateLocation()
  {
    m_gps.m_timestamp += 1.0;
    m_controller->OnLocationUpdate(m_gps, m_context, m_screen);
  }

  void UpdateCompass()
  {
    location::CompassInfo info;
    info.m_bearing = 1.0;
    m_controller->OnCompassUpdate(info, m_screen);
  }

  void Navigate()
  {
    m_controller->ActivateRouting(17 /* zoomLevel */, false /* enableAutoZoom */, false /* isArrowGlued */,
                                  false /* allowRouteRotation */);
    TEST_EQUAL(m_controller->GetCurrentMode(), location::FollowAndRotate, ());
    m_controller->DeactivateRouting();
  }

  void PositionChanged(m2::PointD const &, bool) override {}
  void ChangeModelView(m2::PointD const &, int, df::TAnimationCreator const &) override {}
  void ChangeModelView(double, df::TAnimationCreator const &) override {}
  void ChangeModelView(m2::RectD const &, df::TAnimationCreator const &) override {}
  void ChangeModelView(m2::PointD const &, double, m2::PointD const &, int, df::Animation::TAction const &,
                       df::TAnimationCreator const &) override
  {}
  void ChangeModelView(double, m2::PointD const &, double, m2::PointD const &, df::TAnimationCreator const &) override
  {}

  drape_ptr<df::MyPositionController> m_controller;
  bool m_lastShouldPersist = false;
  ScreenBase m_screen;
  df::NavigationContext m_context;
  location::GpsInfo m_gps;
};
}  // namespace

UNIT_TEST(MyPositionController_NotFollowReturnsToSavedFollowAndRotate)
{
  PositionFixture f(location::NotFollow, location::FollowAndRotate, 0.0);
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());

  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_NotFollowReturnsToSavedFollow)
{
  PositionFixture f(location::NotFollow, location::Follow, 0.0);
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());

  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_LongBackgroundStartsInSavedFollowMode)
{
  PositionFixture f(location::NotFollow, location::FollowAndRotate, kLongBackgroundSec);
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_ShortBackgroundKeepsSavedMode)
{
  PositionFixture f(location::NotFollow, location::FollowAndRotate, 60.0);
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());
}

UNIT_TEST(MyPositionController_LongForegroundReturnUsesSavedFollowMode)
{
  PositionFixture f(location::NotFollow, location::FollowAndRotate, 0.0);
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());

  f.m_controller->OnEnterForeground(kLongBackgroundSec);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_NavigationEndRestoresFollow)
{
  PositionFixture f(location::Follow, location::FollowAndRotate, 0.0);
  f.UpdateLocation();
  f.UpdateCompass();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());

  f.Navigate();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_NavigationEndRestoresFollowAndRotate)
{
  PositionFixture f(location::FollowAndRotate, location::FollowAndRotate, 0.0);
  f.UpdateLocation();
  f.UpdateCompass();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());

  f.Navigate();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_NavigationEndWithoutDirectionFallsBackToFollow)
{
  PositionFixture f(location::FollowAndRotate, location::FollowAndRotate, 0.0);
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());

  f.Navigate();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_PanDuringNavigationKeepsFollowAfterwards)
{
  PositionFixture f(location::Follow, location::Follow, 0.0);
  f.UpdateLocation();
  f.UpdateCompass();

  f.m_controller->ActivateRouting(17 /* zoomLevel */, false /* enableAutoZoom */, false /* isArrowGlued */,
                                  false /* allowRouteRotation */);
  f.m_controller->StopLocationFollow();
  f.m_controller->DeactivateRouting();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());

  f.m_controller->StopLocationFollow();
  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_SetFollowModeRotatesWhenHeadingArrives)
{
  PositionFixture f(location::Follow, location::Follow, 0.0);
  f.m_gps.m_bearing = -1.0;
  f.UpdateLocation();
  TEST(!f.m_controller->IsArrowRotationAvailable(), ());

  f.m_controller->SetFollowMode(location::FollowAndRotate, -1 /* zoomLevel */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
  TEST(!f.m_lastShouldPersist, ());

  f.UpdateCompass();
  TEST(f.m_controller->IsArrowRotationAvailable(), ());
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_SetFollowModeWhilePendingAppliesOnFirstFix)
{
  PositionFixture f(location::Follow, location::Follow, 0.0);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::PendingPosition, ());

  f.m_controller->SetFollowMode(location::FollowAndRotate, -1 /* zoomLevel */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::PendingPosition, ());

  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_SetFollowModeKeepsLocationOff)
{
  PositionFixture f(location::NotFollowNoPosition, location::Follow, 0.0);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollowNoPosition, ());

  f.m_controller->SetFollowMode(location::Follow, -1 /* zoomLevel */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollowNoPosition, ());
}

UNIT_TEST(MyPositionController_SetFollowModeInRoutingKeepsPreferredRoutingMode)
{
  PositionFixture f(location::Follow, location::Follow, 0.0);
  f.UpdateLocation();
  f.UpdateCompass();

  f.m_controller->ActivateRouting(17 /* zoomLevel */, false /* enableAutoZoom */, false /* isArrowGlued */,
                                  false /* allowRouteRotation */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());

  f.m_controller->SetFollowMode(location::Follow, -1 /* zoomLevel */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
  TEST(!f.m_lastShouldPersist, ());

  f.m_controller->DeactivateRouting();
  f.m_controller->ActivateRouting(17 /* zoomLevel */, false /* enableAutoZoom */, false /* isArrowGlued */,
                                  false /* allowRouteRotation */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
}

UNIT_TEST(MyPositionController_LeavingCarRestoresPhoneOrientation)
{
  PositionFixture f(location::Follow, location::FollowAndRotate, 0.0);
  f.UpdateLocation();
  f.UpdateCompass();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());

  f.m_controller->SetCarOwnsMode(true);
  f.m_controller->SetFollowMode(location::FollowAndRotate, -1 /* zoomLevel */);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());

  f.m_controller->SetCarOwnsMode(false);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_CarPanKeepsPhoneFollowMode)
{
  PositionFixture f(location::Follow, location::Follow, 0.0);
  f.UpdateLocation();
  f.UpdateCompass();

  f.m_controller->SetCarOwnsMode(true);
  f.m_controller->SetFollowMode(location::FollowAndRotate, -1 /* zoomLevel */);
  f.m_controller->StopLocationFollow();
  f.m_controller->SetCarOwnsMode(false);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());

  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}
