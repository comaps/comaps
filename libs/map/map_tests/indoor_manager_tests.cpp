#include "testing/testing.hpp"

#include "map/indoor_manager.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/scales.hpp"

#include "i18n/string_utf8_multilang.hpp"

#include "geometry/screenbase.hpp"

#include <string>
#include <vector>

namespace indoor_manager_tests
{
using namespace generator::tests_support;

namespace
{
// A small axis-aligned square around |c|. Indoor detection only considers Area features, so tests
// build tiny polygons rather than POIs.
std::vector<m2::PointD> Square(m2::PointD const & c, double half = 0.00003)
{
  return {{c.x - half, c.y - half}, {c.x + half, c.y - half}, {c.x + half, c.y + half}, {c.x - half, c.y + half}};
}

// A diamond (45°-rotated square) with "radius" |r|. Its bounding box has large empty corners, used to
// test that detection looks at the actual geometry rather than the bbox.
std::vector<m2::PointD> Diamond(m2::PointD const & c, double r)
{
  return {{c.x, c.y - r}, {c.x + r, c.y}, {c.x, c.y + r}, {c.x - r, c.y}};
}

// An area feature with a single classificator type and an optional level=* tag.
class TestArea : public TestFeature
{
public:
  TestArea(std::vector<m2::PointD> geometry, base::StringIL const & type, std::string const & level)
    : TestFeature(std::move(geometry), StringUtf8Multilang{}, Type::Area)
    , m_type(classif().GetTypeByPath(type))
  {
    if (!level.empty())
      GetMetadata().Set(feature::Metadata::FMD_LEVEL, level);
  }

  void Serialize(feature::FeatureBuilder & fb) const override
  {
    TestFeature::Serialize(fb);
    fb.AddType(m_type);
  }

  std::string ToDebugString() const override { return "TestArea"; }

private:
  uint32_t m_type;
};
}  // namespace

class IndoorManagerTest : public TestWithCustomMwms
{
public:
  IndoorManagerTest()
    : m_manager([this](m2::RectD const & rect, std::function<void(FeatureType &)> const & fn, int scale)
  { m_dataSource.ForEachInRect(fn, rect, scale); })
  {
    // Synchronous runners: scan and result application happen inline.
    m_manager.SetTaskRunners([](std::function<void()> && task) { task(); },
                             [](std::function<void()> && task) { task(); });

    m_manager.SetLevelsListener([this](std::vector<std::string> const & levels, std::string const & activeLevel)
    {
      m_lastLevels = levels;
      m_lastActiveLevel = activeLevel;
      ++m_notifications;
    });
  }

protected:
  static ScreenBase MakeScreen(m2::PointD const & center, int drawScale)
  {
    df::VisualParams::Init(1.0, 1024);
    ScreenBase screen;
    screen.OnSize(0, 0, 1024, 768);
    screen.SetFromRect(m2::AnyRectD(df::GetRectForDrawScale(drawScale, center)));
    return screen;
  }

  static TestArea MakeIndoorRoom(m2::PointD const & center, std::string const & level)
  {
    return TestArea(Square(center), {"indoor", "room"}, level);
  }

  static TestArea MakePlatform(m2::PointD const & center, std::string const & level)
  {
    return TestArea(Square(center), {"railway", "platform"}, level);
  }

  IndoorManager m_manager;
  std::vector<std::string> m_lastLevels;
  std::string m_lastActiveLevel;
  size_t m_notifications = 0;
};

UNIT_CLASS_TEST(IndoorManagerTest, ViewportLevels)
{
  m2::PointD const center(0.0, 0.0);

  auto room0 = MakeIndoorRoom(center, "0");
  auto room1 = MakeIndoorRoom(m2::PointD(0.0001, 0.0001), "1");
  auto hallMulti = MakeIndoorRoom(m2::PointD(-0.0001, -0.0001), "0;2");
  auto roomNoLevel = MakeIndoorRoom(m2::PointD(0.0002, 0.0), "");

  BuildCountry("IndoorLand", [&](TestMwmBuilder & builder)
  {
    builder.Add(room0);
    builder.Add(room1);
    builder.Add(hallMulti);
    builder.Add(roomNoLevel);
  });

  // Below the indoor zoom threshold: no levels reported.
  m_manager.UpdateViewport(MakeScreen(center, 15));
  TEST(m_lastLevels.empty(), ());
  TEST(m_manager.GetViewportLevels().empty(), ());

  // At z17+: distinct sorted levels, topmost floor first.
  m_manager.UpdateViewport(MakeScreen(center, 17));
  TEST_EQUAL(m_lastLevels, std::vector<std::string>({"2", "1", "0"}), ());

  // Ground floor is the default active level.
  TEST_EQUAL(m_manager.GetActiveLevel(), "0", ());
  TEST_EQUAL(m_lastActiveLevel, "0", ());

  // Selecting another level keeps the list and switches the active level.
  m_manager.SelectLevel("2");
  TEST_EQUAL(m_manager.GetActiveLevel(), "2", ());

  // Selecting garbage is ignored.
  m_manager.SelectLevel("penthouse");
  TEST_EQUAL(m_manager.GetActiveLevel(), "2", ());

  // Zooming out clears the levels and hides the UI.
  m_manager.UpdateViewport(MakeScreen(center, 14));
  TEST(m_lastLevels.empty(), ());
}

UNIT_CLASS_TEST(IndoorManagerTest, EntryGate)
{
  m2::PointD const center(0.0, 0.0);

  auto room0 = MakeIndoorRoom(center, "0");
  auto room1 = MakeIndoorRoom(m2::PointD(0.0001, 0.0001), "1");

  BuildCountry("IndoorLand", [&](TestMwmBuilder & builder)
  {
    builder.Add(room0);
    builder.Add(room1);
  });

  // Entry disallowed (e.g. driving during navigation): indoor data is ignored, mode stays inactive.
  bool canEnter = false;
  m_manager.SetCanEnterPredicate([&canEnter]() { return canEnter; });
  m_manager.UpdateViewport(MakeScreen(center, 17));
  TEST(m_manager.GetViewportLevels().empty(), ());

  // Once entry is allowed (e.g. on foot), indoor mode activates normally.
  canEnter = true;
  m_manager.Invalidate();
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"1", "0"}), ());

  // An already-active context is preserved even if entry becomes disallowed again: a scan while
  // still inside keeps the levels (the predicate isn't consulted once active).
  canEnter = false;
  m_manager.Invalidate();
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"1", "0"}), ());
}

UNIT_CLASS_TEST(IndoorManagerTest, HoldDuringRouting)
{
  m2::PointD const center(0.0, 0.0);

  auto room0 = MakeIndoorRoom(center, "0");
  auto room1 = MakeIndoorRoom(m2::PointD(0.0001, 0.0001), "1");

  BuildCountry("IndoorLand", [&](TestMwmBuilder & builder)
  {
    builder.Add(room0);
    builder.Add(room1);
  });

  // Enter indoors normally.
  m_manager.UpdateViewport(MakeScreen(center, 17));
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"1", "0"}), ());

  // Routing starts (hold on): zooming out to fit the route must NOT drop the active indoor context.
  bool hold = true;
  m_manager.SetShouldHoldPredicate([&hold]() { return hold; });
  m_manager.UpdateViewport(MakeScreen(center, 14));
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"1", "0"}), ());

  // Routing ends (hold off): the same zoomed-out viewport now deactivates as usual.
  hold = false;
  m_manager.UpdateViewport(MakeScreen(center, 14));
  TEST(m_manager.GetViewportLevels().empty(), ());
}

UNIT_CLASS_TEST(IndoorManagerTest, ClosestToGroundWhenNoGroundFloor)
{
  m2::PointD const center(0.0, 0.0);

  // A building with no level=0: -2, -1 and 3. The floor closest to ground is -1.
  auto down2 = MakeIndoorRoom(center, "-2");
  auto down1 = MakeIndoorRoom(m2::PointD(0.0001, 0.0001), "-1");
  auto up3 = MakeIndoorRoom(m2::PointD(-0.0001, -0.0001), "3");

  BuildCountry("IndoorLand", [&](TestMwmBuilder & builder)
  {
    builder.Add(down2);
    builder.Add(down1);
    builder.Add(up3);
  });

  // On entering, the active floor is the one closest to 0 rather than the lowest available.
  m_manager.UpdateViewport(MakeScreen(center, 17));
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"3", "-1", "-2"}), ());
  TEST_EQUAL(m_manager.GetActiveLevel(), "-1", ());
  TEST_EQUAL(m_lastActiveLevel, "-1", ());
}

UNIT_CLASS_TEST(IndoorManagerTest, LeveledPlatformIsIndoor)
{
  m2::PointD const center(0.0, 0.0);

  // A transit platform on level 2 (the only feature on that floor) plus a surface platform with no
  // level tag. Only the leveled one should register as an indoor level.
  auto leveledPlatform = MakePlatform(center, "2");
  auto surfacePlatform = MakePlatform(m2::PointD(0.0002, 0.0), "");

  BuildCountry("PlatformLand", [&](TestMwmBuilder & builder)
  {
    builder.Add(leveledPlatform);
    builder.Add(surfacePlatform);
  });

  m_manager.UpdateViewport(MakeScreen(center, 17));
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"2"}), ());
  TEST_EQUAL(m_manager.GetActiveLevel(), "2", ());
}

UNIT_CLASS_TEST(IndoorManagerTest, GeometryNotBoundingBox)
{
  m2::PointD const center(0.0, 0.0);
  double const r = 0.02;

  // A diamond-shaped indoor area on level 1. Its bounding box spans [-r, r]^2 but the NE corner is
  // empty space outside the polygon.
  TestArea diamond(Diamond(center, r), {"indoor", "room"}, "1");

  BuildCountry("DiamondLand", [&](TestMwmBuilder & builder) { builder.Add(diamond); });

  // A viewport over the empty NE bbox corner (outside the diamond) must NOT trigger indoor mode,
  // even though the feature's bounding box covers that corner.
  m2::PointD const corner(r * 0.9, r * 0.9);
  m_manager.UpdateViewport(MakeScreen(corner, 18));
  TEST(m_manager.GetViewportLevels().empty(), ("A blank bbox corner must not trigger indoor mode"));

  // A viewport over the diamond body does trigger it.
  m_manager.UpdateViewport(MakeScreen(center, 18));
  TEST_EQUAL(m_manager.GetViewportLevels(), std::vector<std::string>({"1"}), ());
}

UNIT_CLASS_TEST(IndoorManagerTest, NoIndoorData)
{
  m2::PointD const center(5.0, 5.0);

  TestPOI cafe(center, "plain cafe", "en");
  cafe.SetTypes({{"amenity", "cafe"}});

  BuildCountry("OutdoorLand", [&](TestMwmBuilder & builder) { builder.Add(cafe); });

  m_manager.UpdateViewport(MakeScreen(center, 17));
  TEST(m_manager.GetViewportLevels().empty(), ());
}
}  // namespace indoor_manager_tests
