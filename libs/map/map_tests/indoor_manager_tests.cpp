#include "testing/testing.hpp"

#include "map/indoor_manager.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/scales.hpp"

#include "geometry/screenbase.hpp"

#include <string>
#include <vector>

namespace indoor_manager_tests
{
using namespace generator::tests_support;

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

  static TestPOI MakeIndoorPoi(m2::PointD const & center, std::string const & name, std::string const & level)
  {
    TestPOI poi(center, name, "en");
    poi.SetTypes({{"indoor", "room"}});
    if (!level.empty())
      poi.GetMetadata().Set(feature::Metadata::FMD_LEVEL, level);
    return poi;
  }

  IndoorManager m_manager;
  std::vector<std::string> m_lastLevels;
  std::string m_lastActiveLevel;
  size_t m_notifications = 0;
};

UNIT_CLASS_TEST(IndoorManagerTest, ViewportLevels)
{
  m2::PointD const center(0.0, 0.0);

  auto poi0 = MakeIndoorPoi(center, "room ground", "0");
  auto poi1 = MakeIndoorPoi(m2::PointD(0.0001, 0.0001), "room first", "1");
  auto poiMulti = MakeIndoorPoi(m2::PointD(-0.0001, -0.0001), "hall", "0;2");
  auto poiNoLevel = MakeIndoorPoi(m2::PointD(0.0002, 0.0), "room unlabeled", "");

  BuildCountry("IndoorLand", [&](TestMwmBuilder & builder)
  {
    builder.Add(poi0);
    builder.Add(poi1);
    builder.Add(poiMulti);
    builder.Add(poiNoLevel);
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
