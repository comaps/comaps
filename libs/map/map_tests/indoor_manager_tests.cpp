#include "testing/testing.hpp"

#include "map/indoor_manager.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/any_rect2d.hpp"

#include <deque>
#include <vector>

namespace indoor_manager_tests
{
// Runs tasks only when told to, so the async path the manager actually uses is exercised.
class Queue
{
public:
  IndoorManager::TaskRunnerFn Runner()
  {
    return [this](std::function<void()> && task) { m_tasks.push_back(std::move(task)); };
  }

  bool Empty() const { return m_tasks.empty(); }

  void RunOne()
  {
    auto task = std::move(m_tasks.front());
    m_tasks.pop_front();
    task();
  }

  void RunAll()
  {
    while (!m_tasks.empty())
      RunOne();
  }

private:
  std::deque<std::function<void()>> m_tasks;
};

ScreenBase MakeScreen(m2::PointD const & center, int drawScale)
{
  df::VisualParams::Init(1.0, 1024);
  ScreenBase screen;
  screen.OnSize(0, 0, 1024, 768);
  screen.SetFromRect(m2::AnyRectD(df::GetRectForDrawScale(drawScale, center)));
  return screen;
}

// With no features at all the manager must still drive its scan state machine correctly.
IndoorManager::ForEachFeatureFn EmptySource()
{
  return [](m2::RectD const &, std::function<void(FeatureType &)> const &, int) {};
}

// A stale scan used to jam the in-flight slot, and no scan ever ran again for the whole session.
UNIT_TEST(IndoorManager_StaleScanReleasesTheInFlightSlot)
{
  classificator::Load();

  Queue background, ui;
  IndoorManager manager(EmptySource(), background.Runner(), ui.Runner());

  manager.UpdateViewport(MakeScreen(m2::PointD(0.0, 0.0), 18));
  TEST(!background.Empty(), ("a scan should be queued"));

  // Zoom out below the indoor threshold, invalidating the queued scan.
  manager.UpdateViewport(MakeScreen(m2::PointD(0.0, 0.0), 10));
  background.RunAll();
  ui.RunAll();

  // Back in, so a fresh scan must be queued rather than swallowed by an unreleased slot.
  manager.UpdateViewport(MakeScreen(m2::PointD(0.0, 0.0), 18));
  TEST(!background.Empty(), ("the scan pipeline must not wedge after a stale scan"));
}

UNIT_TEST(IndoorManager_CoalescesWhileAScanIsInFlight)
{
  classificator::Load();

  Queue background, ui;
  IndoorManager manager(EmptySource(), background.Runner(), ui.Runner());

  manager.UpdateViewport(MakeScreen(m2::PointD(0.0, 0.0), 18));
  manager.UpdateViewport(MakeScreen(m2::PointD(0.001, 0.0), 18));
  manager.UpdateViewport(MakeScreen(m2::PointD(0.002, 0.0), 18));

  // Only one scan in flight, and the newest center replaces the pending one.
  TEST_EQUAL(1, [&background]() { int n = 0; while (!background.Empty()) { background.RunOne(); ++n; } return n; }(),
             ());

  ui.RunAll();
  background.RunAll();
  ui.RunAll();
  TEST(background.Empty() && ui.Empty(), ("the pipeline should drain"));
}

UNIT_TEST(IndoorManager_InactiveWithoutIndoorData)
{
  classificator::Load();

  Queue background, ui;
  IndoorManager manager(EmptySource(), background.Runner(), ui.Runner());

  manager.UpdateViewport(MakeScreen(m2::PointD(0.0, 0.0), 18));
  background.RunAll();
  ui.RunAll();

  TEST(!manager.IsActive(), ());
  TEST(manager.GetViewportLevels().empty(), ());
  TEST(manager.GetComplex() == nullptr, ());
}

// The manager has no opinion on why it is suspended. It obeys the switch and comes back.
UNIT_TEST(IndoorManager_SuspendedStopsScanningAndResumes)
{
  classificator::Load();

  Queue background, ui;
  IndoorManager manager(EmptySource(), background.Runner(), ui.Runner());

  manager.SetSuspended(true);
  manager.UpdateViewport(MakeScreen(m2::PointD(0.0, 0.0), 18));
  TEST(background.Empty(), ("no scan may be queued while suspended"));
  TEST(!manager.IsActive(), ());

  manager.SetSuspended(false);
  TEST(!background.Empty(), ("releasing the switch rescans the viewport it already knows"));

  // Toggling to the same value must not churn.
  background.RunAll();
  ui.RunAll();
  manager.SetSuspended(false);
  TEST(background.Empty(), ("no rescan when nothing changed"));
}

// A level the active building doesn't have must never reach drape.
UNIT_TEST(IndoorManager_SelectLevelIgnoresUnknownLevels)
{
  classificator::Load();

  Queue background, ui;
  IndoorManager manager(EmptySource(), background.Runner(), ui.Runner());

  TEST(!manager.SelectLevel(42.0), ("no complex, so nothing to select"));
  TEST(!manager.IsActive(), ());
}

// Features as the renderer sees them, stated rather than re-derived, so a MakeFeatureView bug shows.
indoor::FeatureView Room(FeatureID const & id, m2::RectD const & rect, std::string const & level)
{
  indoor::FeatureView f;
  f.m_id = id;
  f.m_rect = rect;
  f.m_level = level;
  f.m_isLeveled = !level.empty();
  f.m_isArea = true;
  f.m_isIndoor = true;
  return f;
}

indoor::FeatureView Poi(m2::RectD const & rect, std::string const & level)
{
  indoor::FeatureView f;
  f.m_rect = rect;
  f.m_level = level;
  f.m_isLeveled = !level.empty();
  return f;
}

indoor::FeatureView Outline(FeatureID const & id, m2::RectD const & rect, bool isPart)
{
  indoor::FeatureView f;
  f.m_id = id;
  f.m_rect = rect;
  f.m_isArea = true;
  f.m_isPart = isPart;
  f.m_isBuilding = true;
  return f;
}

std::shared_ptr<indoor::Complex> UnitComplex()
{
  auto c = std::make_shared<indoor::Complex>();
  c->m_rect = m2::RectD(-1.0, -1.0, 1.0, 1.0);
  c->m_triangles = {m2::TriangleD({-1.0, -1.0}, {1.0, -1.0}, {1.0, 1.0}),
                    m2::TriangleD({-1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0})};
  c->m_levels = {0.0, 1.0};
  return c;
}

// The filter must be able to say "skip", or it is not filtering anything.
UNIT_TEST(IndoorFilter_SkipsWrongFloorInsideActiveComplex)
{
  auto const complex = UnitComplex();
  m2::RectD const inside(-0.1, -0.1, 0.1, 0.1);
  m2::RectD const outside(5.0, 5.0, 5.1, 5.1);

  indoor::Active const onFloor1{complex, 1.0};
  indoor::Active const onFloor0{complex, 0.0};
  indoor::Active const off;

  TEST(!onFloor1.Hides(Room(FeatureID(), inside, "1")), ("visible on its own floor"));
  TEST(onFloor0.Hides(Room(FeatureID(), inside, "1")), ("hidden on another floor"));
  // A non-indoor leveled feature outside the complex is ordinary map content, so leave it alone.
  TEST(!onFloor0.Hides(Poi(outside, "1")), ());
  // With indoor mode off, indoor content is hidden outright rather than drawn as ordinary geometry.
  TEST(off.Hides(Room(FeatureID(), inside, "1")), ());
  // Another building's indoor content stays hidden too.
  TEST(onFloor1.Hides(Room(FeatureID(), outside, "1")), ());
}

// Indoor content must never sit under a 3D building, and flattening compares actual polygons.
UNIT_TEST(IndoorFilter_FlattensWhatSharesGroundWithTheComplex)
{
  auto complex = std::make_shared<indoor::Complex>();
  complex->m_rect = m2::RectD(0.0, 0.0, 0.002, 0.002);
  complex->m_triangles = {m2::TriangleD({0.0, 0.0}, {0.002, 0.0}, {0.002, 0.002}),
                          m2::TriangleD({0.0, 0.0}, {0.002, 0.002}, {0.0, 0.002})};
  indoor::Active const active{complex, 0.0};

  auto const square = [](m2::PointD const & lb, double size)
  {
    m2::PointD const rb(lb.x + size, lb.y), rt(lb.x + size, lb.y + size), lt(lb.x, lb.y + size);
    return std::vector<m2::TriangleD>{m2::TriangleD(lb, rb, rt), m2::TriangleD(lb, rt, lt)};
  };

  // A roof over the complex is flattened.
  TEST(active.Flattens(square({0.0005, 0.0005}, 0.001)), ());
  // Friedrich-Busch-Haus has its center outside but covers the complex, so it must lose its 3D.
  TEST(active.Flattens(square({0.0015, 0.0015}, 0.004)), ());
  // The Edge Grand Central case, where a neighbor standing clear of the complex keeps its 3D.
  TEST(!active.Flattens(square({0.0021, 0.0021}, 0.002)), ());
  TEST(!active.Flattens(square({0.01, 0.01}, 0.001)), ());
  // Nothing is flattened when indoor mode is off.
  TEST(!indoor::Active().Flattens(square({0.0005, 0.0005}, 0.001)), ());
}

// Berlin's level=2 Stadtbahn crosses the station with its center far outside, so Owns misses it.
UNIT_TEST(IndoorFilter_HidesALeveledLineCrossingTheComplex)
{
  auto const complex = UnitComplex();
  // Spans the complex and reaches past both sides, so what matters is that part of it covers it.
  m2::RectD const crossing(-8.0, -0.2, 8.0, 0.2);
  m2::RectD const alongside(-8.0, 4.0, 8.0, 4.4);

  indoor::FeatureView line;
  line.m_rect = crossing;
  line.m_level = "2";
  line.m_isLeveled = true;

  TEST(indoor::Active({complex, 0.0}).Hides(line), ("a level=2 line must not draw on floor 0"));
  TEST(!indoor::Active({complex, 2.0}).Hides(line), ("but it belongs on floor 2"));

  line.m_rect = alongside;
  TEST(!indoor::Active({complex, 0.0}).Hides(line), ("a line clear of the complex is ordinary map content"));
}

// railway=platform is background (priority -1890), yet indoors it is the floor you stand on.
UNIT_TEST(IndoorFilter_PlatformOfTheComplexDrawsAsAFloor)
{
  auto const complex = UnitComplex();

  indoor::FeatureView platform;
  platform.m_rect = m2::RectD(-0.5, -0.5, 0.5, 0.5);
  platform.m_isArea = true;
  platform.m_isPlatform = true;
  platform.m_isLeveled = true;
  platform.m_level = "-2";

  TEST(indoor::Active({complex, -2.0}).DrawsAsFloor(platform), ());
  // Off the complex it is ordinary background again.
  auto elsewhere = platform;
  elsewhere.m_rect = m2::RectD(5.0, 5.0, 5.5, 5.5);
  TEST(!indoor::Active({complex, -2.0}).DrawsAsFloor(elsewhere), ());
  // And with indoor mode off, nothing is redrawn as a floor.
  TEST(!indoor::Active().DrawsAsFloor(platform), ());
  // A platform with no level is not part of anyone's inside.
  auto unleveled = platform;
  unleveled.m_isLeveled = false;
  unleveled.m_level.clear();
  TEST(!indoor::Active({complex, -2.0}).DrawsAsFloor(unleveled), ());
}

// level=-2 rail is layer=-2, worth -2000 depth, so the shown floor must order it and not layer=*.
UNIT_TEST(IndoorFilter_IgnoresLayerOnTheShownFloor)
{
  auto const complex = UnitComplex();

  indoor::FeatureView rail;
  rail.m_rect = m2::RectD(-2.0, -0.2, 2.0, 0.2);
  rail.m_level = "-2";
  rail.m_isLeveled = true;
  rail.m_layer = -2;

  TEST(indoor::Active({complex, -2.0}).IgnoresLayer(rail), ("its level places it, not its layer"));
  TEST(!indoor::Active().IgnoresLayer(rail), ("indoor mode off leaves layer=* alone"));

  auto away = rail;
  away.m_rect = m2::RectD(5.0, 5.0, 6.0, 5.2);
  TEST(!indoor::Active({complex, -2.0}).IgnoresLayer(away), ("a line clear of the complex keeps its layer"));

  // Something with no level of its own is surface detail, handled by sinking instead.
  auto street = rail;
  street.m_isLeveled = false;
  street.m_level.clear();
  TEST(!indoor::Active({complex, -2.0}).IgnoresLayer(street), ());
}

// Unlevelled surface detail is on the ground, so it sinks below other floors but not below floor 0.
UNIT_TEST(IndoorFilter_SinksSurfaceDetailWhileBelowGround)
{
  auto const complex = UnitComplex();

  indoor::FeatureView street;
  street.m_rect = m2::RectD(-2.0, -0.2, 2.0, 0.2);

  TEST(indoor::Active({complex, -2.0}).Sinks(street), ("the street above must not cover level -2"));
  TEST(indoor::Active({complex, 2.0}).Sinks(street), ("nor a floor above the ground"));
  TEST(!indoor::Active({complex, 0.0}).Sinks(street), ("on the ground floor it keeps its priority"));
  TEST(!indoor::Active().Sinks(street), ("and indoor mode off changes nothing"));

  // Clear of the complex, it is ordinary map content on every floor.
  auto away = street;
  away.m_rect = m2::RectD(5.0, 5.0, 6.0, 5.2);
  TEST(!indoor::Active({complex, -2.0}).Sinks(away), ());

  // Anything carrying a level of its own is judged by that level, not sunk.
  auto leveled = street;
  leveled.m_isLeveled = true;
  leveled.m_level = "-2";
  TEST(!indoor::Active({complex, -2.0}).Sinks(leveled), ());
}

// Roofs and parts over the complex go, its own outlines stay, and neighbors are untouched.
UNIT_TEST(IndoorFilter_HidesCoveringPartsAndKeepsTheOutline)
{
  auto const complex = UnitComplex();
  FeatureID const outline(MwmSet::MwmId(), 7);
  complex->m_id = outline;
  complex->m_members = {outline};
  indoor::Active const active{complex, 0.0};

  m2::RectD const over(-0.5, -0.5, 0.5, 0.5);
  m2::RectD const beside(5.0, 5.0, 5.5, 5.5);

  TEST(!active.Hides(Outline(outline, over, false)), ("the complex outline stays"));
  TEST(active.Hides(Outline(FeatureID(MwmSet::MwmId(), 8), over, true)),
       ("a part stacked over the complex is hidden"));
  TEST(active.Hides(Outline(FeatureID(MwmSet::MwmId(), 9), over, false)),
       ("so is a roof mapped as its own building"));
  TEST(!active.Hides(Outline(FeatureID(MwmSet::MwmId(), 10), beside, false)),
       ("a building beside the complex is left alone"));
  TEST(!indoor::Active().Hides(Outline(outline, over, false)), ("indoor mode off"));
}
}  // namespace indoor_manager_tests
