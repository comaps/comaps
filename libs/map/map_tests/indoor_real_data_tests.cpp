#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/indoor_manager.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/indoor.hpp"
#include "indexer/scales.hpp"

#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/mercator.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// Test indoor scanning and filtering against real MWM fixture data
//
// You should regen the MWM fixtures if the indoor type ids in data/mapcss-mapping.csv change, or when the
// MWM format changes. The MWMs were generated with indoor|room 532, indoor|corridor 533, indoor|area 534,
// indoor|wall 535, indoor|door 536, indoor|level 537, highway|corridor 538.
//
// generator_tool should use a Release or RelWithDebInfo build, Debug builds often have issues with ASSERTs.
//
//   --complete-multipolygons is required because Berlin Hauptbahnhof's platforms are multipolygon
//   relations that vanish from the extract without it. Do NOT add --complete-boundaries, which
//   pulls in a partially clipped administrative boundary and aborts the generator's place processor.
//
//   osmconvert berlin-latest.osm.pbf -b=13.359,52.518,13.387,52.536 --complete-ways --complete-multipolygons -o=berlin.osm
//   osmconvert ile-de-france-latest.osm.pbf -b=2.578,48.834,2.598,48.848 --complete-ways --complete-multipolygons -o=descartes.osm
//
//   borders/<name>.poly must match the bbox, or the generator clips everything away.
//
//   generator_tool --user_resource_path=data/ --intermediate_data_path=int/ --osm_file_type=xml --osm_file_name=berlin.osm --preprocess
//   generator_tool --user_resource_path=data/ --intermediate_data_path=int/ --data_path=out/ --osm_file_type=xml --osm_file_name=berlin.osm --generate_features --generate_geometry --generate_index --output=indoor-berlin
//
//   generator_tool --user_resource_path=data/ --intermediate_data_path=int/ --osm_file_type=xml --osm_file_name=descartes.osm --preprocess
//   generator_tool --user_resource_path=data/ --intermediate_data_path=int/ --data_path=out/ --osm_file_type=xml --osm_file_name=descartes.osm --generate_features --generate_geometry --generate_index --output=indoor-descartes
//
// No search index, routing section or cross-MWM required. These tests only read features and geometry,
// and omitting the rest keeps the fixtures small enough for git.
//
// Verify a regenerated fixture works by itself by moving any downloaded maps aside before running:
//
//   mv data/260714 data/260714.aside
//   map_tests --filter=IndoorRealData
//   mv data/260714.aside data/260714
namespace indoor_real_data_tests
{
struct Place
{
  char const * m_country;
  double m_lat;
  double m_lon;
};

Place const kCiteDescartes = {"indoor-descartes", 48.84122, 2.58790};
Place const kBerlinHbf = {"indoor-berlin", 52.52506, 13.36937};
Place const kCharite = {"indoor-berlin", 52.5282, 13.3763};

bool HasMap(Place const & place)
{
  return GetPlatform().IsFileExistsByFullPath(GetPlatform().WritablePathForFile(
      std::string(place.m_country) + DATA_FILE_EXTENSION));
}

ScreenBase MakeScreen(m2::PointD const & center, int drawScale)
{
  df::VisualParams::Init(1.0, 1024);
  ScreenBase screen;
  screen.OnSize(0, 0, 1024, 768);
  screen.SetFromRect(m2::AnyRectD(df::GetRectForDrawScale(drawScale, center)));
  return screen;
}

// The Framework's own manager scans on background threads a test never runs, so drive our own.
IndoorManager MakeSyncManager(Framework & frm)
{
  auto const & dataSource = frm.GetDataSource();
  auto run = [](std::function<void()> && task) { task(); };
  return IndoorManager([&dataSource](m2::RectD const & rect, std::function<void(FeatureType &)> const & fn, int scale)
  { dataSource.ForEachInRect(fn, rect, scale); }, run, run);
}

void CheckPlace(Place const & place)
{
  if (!HasMap(place))
  {
    LOG(LWARNING, ("Skipping, map not installed:", place.m_country));
    return;
  }

  Framework frm(FrameworkParams(false /* m_enableDiffs */));
  frm.RegisterMap(platform::LocalCountryFile::MakeForTesting(place.m_country));

  auto manager = MakeSyncManager(frm);
  // A grid, so one unlucky point in a courtyard is not mistaken for broken detection.
  size_t activated = 0;
  std::shared_ptr<indoor::Complex const> building;
  m2::PointD bestCentre;
  double const kStep = 0.0004;  // ~45 m
  for (int dy = -2; dy <= 2; ++dy)
  {
    for (int dx = -2; dx <= 2; ++dx)
    {
      auto const centre = mercator::FromLatLon(place.m_lat + dy * kStep, place.m_lon + dx * kStep);
      auto probe = MakeSyncManager(frm);
      probe.UpdateViewport(MakeScreen(centre, 18));
      auto const b = probe.GetComplex();
      if (!b)
        continue;
      ++activated;
      if (!building || b->m_levels.size() > building->m_levels.size())
      {
        building = b;
        bestCentre = centre;
      }
    }
  }
  LOG(LINFO, (place.m_country, "activated at", activated, "of 25 probe points"));

  auto const levels = building ? building->m_levels : std::vector<double>();

  LOG(LINFO, (place.m_country, "levels:", levels.size()));
  if (!building)
  {
    // No indoor data around this point, so there is nothing to assert about filtering.
    LOG(LWARNING, ("Skipping, no indoor data near", place.m_country));
    return;
  }
  TEST_GREATER_OR_EQUAL(levels.size(), 2, ("a real complex has several floors"));

  // Every resolved floor must be reachable on the manager that actually found this building.
  manager.UpdateViewport(MakeScreen(bestCentre, 18));
  for (double const level : levels)
  {
    TEST(manager.SelectLevel(level), ("SelectLevel must accept a resolved floor"));
    TEST_EQUAL(manager.GetActiveLevelValue(), level, ());
  }

  // Walk the features inside the building and count what the filter would draw per floor.
  auto const & dataSource = frm.GetDataSource();
  std::vector<size_t> drawnPerLevel(levels.size(), 0);
  size_t leveled = 0;

  dataSource.ForEachInRect(
      [&](FeatureType & ft)
  {
    feature::TypesHolder const types(ft);
    auto const view = indoor::MakeFeatureView(ft, types, scales::GetUpperScale(), false /* withGeometry */);
    if (!view.NamesAFloor() || !building->Owns(view.m_rect))
      return;

    ++leveled;
    for (size_t i = 0; i < levels.size(); ++i)
      if (!indoor::Active{building, levels[i]}.Hides(view))
        ++drawnPerLevel[i];
  }, building->m_rect, scales::GetUpperScale());

  // What does the resolved complex actually cover, and what near it stays 3D?
  {
    double const wM = mercator::DistanceOnEarth(building->m_rect.LeftBottom(), building->m_rect.RightBottom());
    double const hM = mercator::DistanceOnEarth(building->m_rect.LeftBottom(), building->m_rect.LeftTop());
    LOG(LINFO, ("  complex extent m:", (int)wM, "x", (int)hM, "triangles:", building->m_triangles.size()));
  }

  LOG(LINFO, (place.m_country, "leveled features inside building:", leveled));
  for (size_t i = 0; i < levels.size(); ++i)
    LOG(LINFO, ("  level", levels[i], "draws", drawnPerLevel[i], "of", leveled));

  TEST_GREATER(leveled, 0, ("the building must contain level-tagged features"));

  // The point of the feature, where each floor shows some content and never all of it.
  size_t drawnEverywhere = 0;
  for (size_t i = 0; i < levels.size(); ++i)
  {
    TEST_GREATER(drawnPerLevel[i], 0, ("floor", levels[i], "must show something"));
    if (drawnPerLevel[i] == leveled)
      ++drawnEverywhere;
  }
  TEST_LESS(drawnEverywhere, levels.size(), ("at least one floor must hide something, or nothing is filtered"));
}

// The hard requirement, that nothing drawn indoors is ever covered by a building left in 3D.
void CheckNothingIndoorUnder3d(Place const & place)
{
  if (!HasMap(place))
  {
    LOG(LWARNING, ("Skipping, map not installed:", place.m_country));
    return;
  }

  Framework frm(FrameworkParams(false /* m_enableDiffs */));
  frm.RegisterMap(platform::LocalCountryFile::MakeForTesting(place.m_country));

  auto manager = MakeSyncManager(frm);
  manager.UpdateViewport(MakeScreen(mercator::FromLatLon(place.m_lat, place.m_lon), 18));
  auto const building = manager.GetComplex();
  if (!building)
  {
    LOG(LWARNING, ("Skipping, indoor mode does not activate at", place.m_lat, place.m_lon));
    return;
  }

  auto const & ds = frm.GetDataSource();
  // Every building around the complex that keeps its 3D under the flattening rule.
  std::vector<indoor::Complex> stillIn3d;
  ds.ForEachInRect([&](FeatureType & ft)
  {
    feature::TypesHolder const types(ft);
    if (ft.GetGeomType() != feature::GeomType::Area)
      return;
    if (!ftypes::IsBuildingChecker::Instance()(types) && !ftypes::IsBuildingPartChecker::Instance()(types))
      return;

    indoor::Complex b;
    b.m_id = ft.GetID();
    b.m_rect = ft.GetLimitRect(scales::GetUpperScale());
    ft.ForEachTriangle([&b](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    { b.m_triangles.emplace_back(p1, p2, p3); }, scales::GetUpperScale());
    if (b.m_triangles.empty() || indoor::Active{building, 0.0}.Flattens(b.m_triangles))
      return;
    stillIn3d.push_back(std::move(b));
  }, building->m_rect, scales::GetUpperScale());

  size_t violations = 0;
  for (double const level : building->m_levels)
  {
    ds.ForEachInRect([&](FeatureType & ft)
    {
      feature::TypesHolder const types(ft);
      if (!ftypes::IsIndoorChecker::Instance()(types))
        return;
      m2::RectD const r = ft.GetLimitRect(scales::GetUpperScale());
      auto const view = indoor::MakeFeatureView(ft, types, scales::GetUpperScale(), false /* withGeometry */);
      if (indoor::Active{building, level}.Hides(view))
        return;

      // Polygon against polygon, so a diagonal room's box does not invent a violation.
      std::vector<m2::TriangleD> indoorTriangles;
      if (ft.GetGeomType() == feature::GeomType::Area)
        ft.ForEachTriangle([&indoorTriangles](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
        { indoorTriangles.emplace_back(p1, p2, p3); }, scales::GetUpperScale());

      for (auto const & b : stillIn3d)
      {
        bool const hit = indoorTriangles.empty() ? b.Reaches(r) : b.OverlapsPolygon(indoorTriangles);
        if (!hit)
          continue;
        ++violations;
        double const w = mercator::DistanceOnEarth(r.LeftBottom(), r.RightBottom());
        double const h = mercator::DistanceOnEarth(r.LeftBottom(), r.LeftTop());
        LOG(LWARNING, ("  REQUIREMENT level", level, "indoor bbox m", (int)w, "x", (int)h, "tris",
                       indoorTriangles.size(), "under building tris", b.m_triangles.size()));
        break;
      }
    }, building->m_rect, scales::GetUpperScale());
  }

  LOG(LINFO, ("  requirement check:", stillIn3d.size(), "buildings still 3D near the complex,", violations,
              "indoor features under them"));
  TEST_EQUAL(violations, 0, ("no indoor element may be drawn under a 3D building"));
}

// Make sure the test data still has the awkward roofs and sprawling floors these tests need.
UNIT_TEST(IndoorRealData_BerlinFixtureHasTheAwkwardGeometry)
{
  if (!HasMap(kBerlinHbf))
  {
    LOG(LWARNING, ("Skipping, fixture not installed:", kBerlinHbf.m_country));
    return;
  }

  Framework frm(FrameworkParams(false /* m_enableDiffs */));
  frm.RegisterMap(platform::LocalCountryFile::MakeForTesting(kBerlinHbf.m_country));

  size_t stackedRoofs = 0, sprawlingIndoor = 0;
  auto const c = mercator::FromLatLon(kBerlinHbf.m_lat, kBerlinHbf.m_lon);
  frm.GetDataSource().ForEachInRect([&](FeatureType & ft)
  {
    feature::TypesHolder const types(ft);
    auto const view = indoor::MakeFeatureView(ft, types, scales::GetUpperScale(), false /* withGeometry */);
    if (!view.m_isArea)
      return;

    if (view.m_isBuilding && view.m_layer >= 3)
      ++stackedRoofs;

    if (view.m_isIndoor)
    {
      double const w = mercator::DistanceOnEarth(view.m_rect.LeftBottom(), view.m_rect.RightBottom());
      double const h = mercator::DistanceOnEarth(view.m_rect.LeftBottom(), view.m_rect.LeftTop());
      if (std::max(w, h) > 150.0)
        ++sprawlingIndoor;
    }
  }, mercator::RectByCenterXYAndSizeInMeters(c, 400.0), scales::GetUpperScale());

  LOG(LINFO, ("  fixture: stacked roofs", stackedRoofs, "sprawling indoor areas", sprawlingIndoor));
  TEST_GREATER(stackedRoofs, 0, ("the station roofs must survive the extract"));
  TEST_GREATER(sprawlingIndoor, 0, ("the underground concourses must survive the extract"));
}

UNIT_TEST(IndoorRealData_NothingIndoorUnder3d_Charite)
{
  CheckNothingIndoorUnder3d(kCharite);
}

UNIT_TEST(IndoorRealData_NothingIndoorUnder3d_BerlinHbf)
{
  CheckNothingIndoorUnder3d(kBerlinHbf);
}

UNIT_TEST(IndoorRealData_CiteDescartes)
{
  CheckPlace(kCiteDescartes);
}

UNIT_TEST(IndoorRealData_BerlinHbf)
{
  CheckPlace(kBerlinHbf);
}
}  // namespace indoor_real_data_tests
