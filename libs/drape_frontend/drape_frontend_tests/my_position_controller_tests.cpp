#include "testing/testing.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/my_position_controller.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/screenbase.hpp"

namespace
{
class PositionFixture : public df::MyPositionController::Listener
{
public:
  PositionFixture()
  {
    df::VisualParams::Init(1.0, 1024);
    m_controller = make_unique_dp<df::MyPositionController>(
        df::MyPositionController::Params(location::Follow, location::FollowAndRotate, 0.0, df::Hints{},
                                        false /* isRoutingActive */, false /* isAutozoomEnabled */,
                                        [](location::EMyPositionMode, bool, bool) {}),
        nullptr /* notifier: these tests do not render or schedule timers */);
    m_controller->SetListener(make_ref(this));
    m_screen.SetFromParams(m2::PointD::Zero(), 0.0, df::GetScreenScale(15));
    m_controller->OnUpdateScreen(m_screen);
    m_context.m_distanceToNextTurn = -1.0;
    m_context.m_speedLimit = -1.0;
    m_gps.m_horizontalAccuracy = 5.0;
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

  void AcquireThenLoseLocation()
  {
    UpdateLocation();
    TEST_EQUAL(m_controller->GetCurrentMode(), location::Follow, ());
    m_controller->LoseLocation();
    TEST_EQUAL(m_controller->GetCurrentMode(), location::PendingPosition, ());
    m_cameraChanges = 0;
  }

  void PositionChanged(m2::PointD const &, bool hasPosition) override { m_hasPosition = hasPosition; }
  void ChangeModelView(m2::PointD const &, int, df::TAnimationCreator const &) override { ++m_cameraChanges; }
  void ChangeModelView(double, df::TAnimationCreator const &) override { ++m_cameraChanges; }
  void ChangeModelView(m2::RectD const &, df::TAnimationCreator const &) override { ++m_cameraChanges; }
  void ChangeModelView(m2::PointD const &, double, m2::PointD const &, int,
                       df::Animation::TAction const &, df::TAnimationCreator const &) override
  {
    ++m_cameraChanges;
  }
  void ChangeModelView(double, m2::PointD const &, double, m2::PointD const &,
                       df::TAnimationCreator const &) override
  {
    ++m_cameraChanges;
  }

  drape_ptr<df::MyPositionController> m_controller;
  ScreenBase m_screen;
  df::NavigationContext m_context;
  location::GpsInfo m_gps;
  int m_cameraChanges = 0;
  bool m_hasPosition = false;
};
}  // namespace

UNIT_TEST(MyPositionController_PanCancelsPendingLocationRecovery)
{
  PositionFixture f;
  f.AcquireThenLoseLocation();
  f.m_controller->StopLocationFollow();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());
  TEST_EQUAL(f.m_cameraChanges, 0, ());

  f.m_gps.m_latitude += 0.001;
  f.UpdateLocation();
  f.m_controller->UpdatePosition();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());
  TEST(f.m_hasPosition, ());
  TEST_EQUAL(f.m_cameraChanges, 0, ());
}

UNIT_TEST(MyPositionController_LocationRecoveryStillFollowsWithoutPan)
{
  PositionFixture f;
  f.AcquireThenLoseLocation();
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
  TEST(f.m_hasPosition, ());
  TEST_GREATER(f.m_cameraChanges, 0, ());
}

UNIT_TEST(MyPositionController_PanBeforeFirstLocationDoesNotRecenter)
{
  PositionFixture f;
  f.m_controller->StopLocationFollow();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::PendingPosition, ());
  f.UpdateLocation();
  f.m_controller->UpdatePosition();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::NotFollow, ());
  TEST(f.m_hasPosition, ());
  TEST_EQUAL(f.m_cameraChanges, 0, ());
}

UNIT_TEST(MyPositionController_RecenterAfterCancellingRecovery)
{
  PositionFixture f;
  f.AcquireThenLoseLocation();
  f.m_controller->StopLocationFollow();
  f.UpdateLocation();
  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
  TEST_GREATER(f.m_cameraChanges, 0, ());
}

UNIT_TEST(MyPositionController_RecenterBeforeLocationRecovers)
{
  PositionFixture f;
  f.AcquireThenLoseLocation();
  f.m_controller->StopLocationFollow();
  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
  TEST_GREATER(f.m_cameraChanges, 0, ());
  f.UpdateLocation();
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::Follow, ());
}

UNIT_TEST(MyPositionController_RecenterPreservesHeadingAfterCancellingRecovery)
{
  PositionFixture f;
  f.m_gps.m_bearing = 90.0;
  f.UpdateLocation();
  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
  f.m_controller->LoseLocation();
  f.m_controller->StopLocationFollow();
  f.m_cameraChanges = 0;
  f.UpdateLocation();
  TEST_EQUAL(f.m_cameraChanges, 0, ());

  f.m_controller->NextMode(f.m_screen);
  TEST_EQUAL(f.m_controller->GetCurrentMode(), location::FollowAndRotate, ());
  TEST_GREATER(f.m_cameraChanges, 0, ());
}
