#include "testing/testing.hpp"

#include "map/indoor_scan.hpp"

#include <algorithm>
#include <vector>

namespace indoor_scan_tests
{
using indoor::FeatureView;

std::vector<m2::TriangleD> SquareTriangles(m2::PointD const & c, double half)
{
  m2::PointD const lb(c.x - half, c.y - half), rb(c.x + half, c.y - half);
  m2::PointD const rt(c.x + half, c.y + half), lt(c.x - half, c.y + half);
  return {m2::TriangleD(lb, rb, rt), m2::TriangleD(lb, rt, lt)};
}

FeatureView MakeBuilding(uint32_t index, m2::PointD const & c, double half)
{
  FeatureView f;
  f.m_id = FeatureID(MwmSet::MwmId(), index);
  f.m_isBuilding = true;
  f.m_isArea = true;
  f.m_rect = m2::RectD(c.x - half, c.y - half, c.x + half, c.y + half);
  f.m_triangles = SquareTriangles(c, half);
  return f;
}

FeatureView MakeRoom(uint32_t index, m2::PointD const & c, double half, std::string const & level)
{
  FeatureView f;
  f.m_id = FeatureID(MwmSet::MwmId(), index);
  f.m_isIndoor = true;
  f.m_isArea = true;
  f.m_level = level;
  f.m_isLeveled = !level.empty();
  f.m_rect = m2::RectD(c.x - half, c.y - half, c.x + half, c.y + half);
  f.m_triangles = SquareTriangles(c, half);
  return f;
}

// A level-tagged POI, e.g. a shop on an upper floor. No geometry, no indoor=* type.
FeatureView MakePoi(uint32_t index, m2::PointD const & c, std::string const & level)
{
  FeatureView f;
  f.m_id = FeatureID(MwmSet::MwmId(), index);
  f.m_isLeveled = true;
  f.m_rect = m2::RectD(c, c);
  f.m_level = level;
  return f;
}

// Yields everything regardless of the rect, since the map index over-fetches and the scan filters.
indoor::ForEachFn Source(std::vector<FeatureView> const & features)
{
  return [&features](m2::RectD const &, indoor::FeatureFn const & fn)
  {
    for (auto const & f : features)
      fn(f);
  };
}

UNIT_TEST(IndoorScan_BuildingWithFloorsActivates)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, "0"),
      MakeRoom(3, c, 0.0004, "1"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST_EQUAL(b->m_levels, std::vector<double>({0.0, 1.0}), ());
  TEST_EQUAL(b->m_id.m_index, 1, ());
}

// The reported bug, where a lone room at the edge of a downtown viewport took over the map.
UNIT_TEST(IndoorScan_RoomAwayFromCenterDoesNotActivate)
{
  m2::PointD const centre(0.0, 0.0);
  m2::PointD const faraway(0.05, 0.05);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, faraway, 0.0005),
      MakeRoom(2, faraway, 0.0002, "0"),
      MakeRoom(3, faraway, 0.0002, "3"),
  };

  TEST(!indoor::ScanForActiveComplex(centre, Source(features), nullptr).has_value(), ());
  // It does activate once that building is actually what you are looking at.
  TEST(indoor::ScanForActiveComplex(faraway, Source(features), nullptr).has_value(), ());
}

UNIT_TEST(IndoorScan_SingleFloorDoesNotActivate)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, "0"),
  };

  TEST(!indoor::ScanForActiveComplex(c, Source(features), nullptr).has_value(), ());
}

UNIT_TEST(IndoorScan_UntaggedRoomsContributeNoFloor)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, ""),
      MakeRoom(3, c, 0.0004, "G"),
  };

  // No phantom ground floor, because unparsable and absent levels name nothing.
  TEST(!indoor::ScanForActiveComplex(c, Source(features), nullptr).has_value(), ());
}

UNIT_TEST(IndoorScan_RoomOutsideBuildingIsNotCounted)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, "0"),
      MakeRoom(3, c, 0.0004, "1"),
      // Inside the building's bbox reach but well outside its polygon.
      MakeRoom(4, m2::PointD(0.01, 0.01), 0.0001, "7"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST_EQUAL(b->m_levels, std::vector<double>({0.0, 1.0}), ());
}

// Berlin Hbf shape, where the shell must win or the footprint jumps from part to part on panning.
UNIT_TEST(IndoorScan_LargestBuildingUnderCenterWins)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.002),
      MakeBuilding(2, c, 0.0005),
      MakeRoom(3, c, 0.0008, "0"),
      MakeRoom(4, c, 0.0008, "2"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST_EQUAL(b->m_id.m_index, 1, ());
}

// Panning across the parts of one complex must keep resolving the same building.
UNIT_TEST(IndoorScan_StableAcrossPartsOfTheSameComplex)
{
  std::vector<FeatureView> const features = {
      MakeBuilding(1, m2::PointD(0.0, 0.0), 0.002),
      MakeBuilding(2, m2::PointD(-0.001, 0.0), 0.0004),
      MakeBuilding(3, m2::PointD(0.001, 0.0), 0.0004),
      MakeRoom(4, m2::PointD(0.0, 0.0), 0.0008, "0"),
      MakeRoom(5, m2::PointD(0.0, 0.0), 0.0008, "1"),
  };

  for (auto const & centre : {m2::PointD(-0.001, 0.0), m2::PointD(0.0, 0.0), m2::PointD(0.001, 0.0)})
  {
    auto const b = indoor::ScanForActiveComplex(centre, Source(features), nullptr);
    TEST(b.has_value(), (centre));
    TEST_EQUAL(b->m_id.m_index, 1, (centre));
  }
}

// Berlin Hbf keeps most of its upper floors on POIs, not on indoor=* areas.
UNIT_TEST(IndoorScan_LevelTaggedPoisContributeFloors)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0005, "0"),
      MakePoi(3, m2::PointD(0.0002, 0.0), "1"),
      MakePoi(4, m2::PointD(-0.0002, 0.0), "2"),
      // Outside the building, so it names no floor of it.
      MakePoi(5, m2::PointD(0.5, 0.5), "9"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST_EQUAL(b->m_levels, std::vector<double>({0.0, 1.0, 2.0}), ());
}

// Nobody centers a building precisely, and odd shapes flicker if we demand it.
UNIT_TEST(IndoorScan_ActivatesJustOutsideTheBuilding)
{
  m2::PointD const c(0.0, 0.0);
  double const half = 0.0005;  // ~55 m across
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, half),
      MakeRoom(2, c, 0.0002, "0"),
      MakeRoom(3, c, 0.0002, "1"),
  };

  // A few meters outside the wall still counts as looking at this building.
  m2::PointD const justOutside(half + 0.00008, 0.0);
  TEST(indoor::ScanForActiveComplex(justOutside, Source(features), nullptr).has_value(), ());

  // Far away it does not, so a building across the street cannot take over.
  m2::PointD const farAway(half + 0.004, 0.0);
  TEST(!indoor::ScanForActiveComplex(farAway, Source(features), nullptr).has_value(), ());
}

// A closet has floors but is not walkable, and activating on it hides every room around it.
UNIT_TEST(IndoorScan_TinyFootprintDoesNotActivate)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.00003),
      MakeRoom(2, c, 0.00001, "0"),
      MakeRoom(3, c, 0.00001, "-1"),
  };

  TEST(!indoor::ScanForActiveComplex(c, Source(features), nullptr).has_value(), ());
}

// A platform line running past belongs to the tracks, so its level is not a floor of the building.
UNIT_TEST(IndoorScan_PassingLineDoesNotDonateItsLevel)
{
  m2::PointD const c(0.0, 0.0);
  FeatureView platform;
  platform.m_id = FeatureID(MwmSet::MwmId(), 4);
  platform.m_isLeveled = true;
  platform.m_rect = m2::RectD(0.0005, 0.0005, 0.02, 0.02);
  platform.m_level = "5";

  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, "0"),
      MakeRoom(3, c, 0.0004, "1"),
      platform,
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST_EQUAL(b->m_levels, std::vector<double>({0.0, 1.0}), ());
}

// One mapped cupboard does not make an office block walkable, and flattens it for nothing.
UNIT_TEST(IndoorScan_StrayRoomDoesNotMakeABuildingIndoor)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.002),
      MakeRoom(2, c, 0.00005, "0"),
      MakeRoom(3, c, 0.00005, "1"),
  };

  TEST(!indoor::ScanForActiveComplex(c, Source(features), nullptr).has_value(), ());
}

// The district-flattening bug, where an ordinary neighbor let the footprint chain across a block.
UNIT_TEST(IndoorScan_OrdinaryNeighbourIsNotGlobbed)
{
  m2::PointD const c(0.0, 0.0);
  m2::PointD const neighbour(0.0019, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeBuilding(2, neighbour, 0.001),
      MakeRoom(3, c, 0.0004, "0"),
      MakeRoom(4, c, 0.0004, "1"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST(!b->Contains(neighbour), ("the neighbour has no indoor content of its own"));
}

// Hbf is several overlapping polygons full of rooms, and left apart one half's roofs cover another.
UNIT_TEST(IndoorScan_OverlappingIndoorBuildingsGlobIntoOneComplex)
{
  m2::PointD const c(0.0, 0.0);
  m2::PointD const half2(0.0015, 0.0);
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeBuilding(2, half2, 0.001),
      MakeRoom(3, c, 0.0004, "0"),
      MakeRoom(4, c, 0.0004, "1"),
      MakeRoom(5, half2, 0.0004, "2"),
      MakeRoom(6, half2, 0.0004, "3"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST(b->Contains(half2), ("both halves belong to the same complex"));
  TEST_EQUAL(b->m_levels, std::vector<double>({0.0, 1.0, 2.0, 3.0}), ());
}

// Hbf's building=roof polygons at layer 3-5 shape the footprint but must never be kept outlines.
UNIT_TEST(IndoorScan_StackedRoofShapesTheComplexButIsNotAnOutline)
{
  m2::PointD const c(0.0, 0.0);
  auto roof = MakeBuilding(2, m2::PointD(0.0015, 0.0), 0.001);
  roof.m_layer = 4;

  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      roof,
      MakeRoom(3, c, 0.0004, "0"),
      MakeRoom(4, c, 0.0004, "1"),
      MakeRoom(5, m2::PointD(0.0015, 0.0), 0.0004, "2"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST(b->Contains(m2::PointD(0.0015, 0.0)), ("the roof still shapes the footprint"));
  TEST(b->IsMember(FeatureID(MwmSet::MwmId(), 1)), ("the ground outline stays drawn"));
  TEST(!b->IsMember(FeatureID(MwmSet::MwmId(), 2)), ("the roof is hidden, not drawn as an outline"));
}

// Denns BioMarkt juts out of the shell, so only overlap reaches it and lets a floor recognize it.
UNIT_TEST(IndoorScan_ProtrudingRoomJoinsTheComplex)
{
  m2::PointD const c(0.0, 0.0);
  m2::PointD const jutting(0.0012, 0.0);  // center outside the building, body overlapping it
  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, "0"),
      MakeRoom(3, c, 0.0004, "1"),
      MakeRoom(4, jutting, 0.0004, "0"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST(b->Contains(m2::PointD(0.0015, 0.0)), ("the protruding room is covered by the complex"));
}

// Hbf's level=-2 platform runs far past the concourse, and uncovered the buildings over it stay 3D.
UNIT_TEST(IndoorScan_LeveledPlatformExtendsTheComplex)
{
  m2::PointD const c(0.0, 0.0);
  m2::PointD const beyond(0.0018, 0.0);

  FeatureView platform;
  platform.m_id = FeatureID(MwmSet::MwmId(), 4);
  platform.m_isPlatform = true;
  platform.m_isArea = true;
  platform.m_isLeveled = true;
  platform.m_level = "-2";
  platform.m_rect = m2::RectD(-0.0005, -0.0003, 0.0025, 0.0003);
  platform.m_triangles = SquareTriangles(m2::PointD(0.001, 0.0), 0.0012);

  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      MakeRoom(2, c, 0.0004, "0"),
      MakeRoom(3, c, 0.0004, "1"),
      platform,
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ());
  TEST(b->Contains(beyond), ("the complex reaches along the platform"));
  TEST(std::find(b->m_levels.begin(), b->m_levels.end(), -2.0) != b->m_levels.end(),
       ("the platform names a floor of the station"));
}

// A station can be mapped with no indoor=* at all, just levelled platforms. That is still an inside.
UNIT_TEST(IndoorScan_PlatformsAloneMakeAStationIndoor)
{
  m2::PointD const c(0.0, 0.0);
  auto platform = [](uint32_t index, m2::PointD const & at, double half, std::string const & level)
  {
    FeatureView f;
    f.m_id = FeatureID(MwmSet::MwmId(), index);
    f.m_isPlatform = true;
    f.m_isArea = true;
    f.m_isLeveled = true;
    f.m_level = level;
    f.m_rect = m2::RectD(at.x - half, at.y - half, at.x + half, at.y + half);
    f.m_triangles = SquareTriangles(at, half);
    return f;
  };

  std::vector<FeatureView> const features = {
      MakeBuilding(1, c, 0.001),
      platform(2, c, 0.0004, "-2"),
      platform(3, c, 0.0004, "0"),
  };

  auto const b = indoor::ScanForActiveComplex(c, Source(features), nullptr);
  TEST(b.has_value(), ("platforms alone are enough to have an inside"));
  TEST_EQUAL(b->m_levels, std::vector<double>({-2.0, 0.0}), ());
}

UNIT_TEST(IndoorScan_NoBuildingMeansNoIndoorMode)
{
  m2::PointD const c(0.0, 0.0);
  std::vector<FeatureView> const features = {
      MakeRoom(1, c, 0.0001, "0"),
      MakeRoom(2, c, 0.0001, "1"),
  };

  TEST(!indoor::ScanForActiveComplex(c, Source(features), nullptr).has_value(), ());
}
}  // namespace indoor_scan_tests
