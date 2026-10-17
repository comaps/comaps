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
                                         [](location::EMyPositionMode, bool, bool) {}),
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
