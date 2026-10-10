#include "testing/testing.hpp"

#include "routing/routing_tests/road_graph_builder.hpp"
#include "routing/routing_tests/tools.hpp"

#include "routing/car_directions.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/route.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/routing_settings.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"
#include "routing/turns_generator_utils.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/macros.hpp"

#include <string>
#include <vector>

namespace routing
{
void GetTurnDirectionBasic(turns::IRoutingResult const & result, size_t outgoingSegmentIndex,
                           NumMwmIds const & numMwmIds, RoutingSettings const & vehicleSettings, turns::TurnItem & turn,
                           turns::CarDirection & previousDirection);
}  // namespace routing

namespace turn_generator_test
{
using namespace routing;
using namespace std;
using namespace turns;

// It's a dummy class to wrap |segments| for tests.
class RoutingResultTest : public IRoutingResult
{
public:
  explicit RoutingResultTest(TUnpackedPathSegments const & segments) : m_segments(segments) {}

  void SetPossibleTurns(size_t const ingoingCount, TurnCandidates const & turns)
  {
    m_ingoingCount = ingoingCount;
    m_possibleTurns = turns;
    m_hasPossibleTurns = true;
  }

  TUnpackedPathSegments const & GetSegments() const override { return m_segments; }

  void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint, size_t & ingoingCount,
                        TurnCandidates & outgoingTurns) const override
  {
    if (m_hasPossibleTurns)
    {
      ingoingCount = m_ingoingCount;
      outgoingTurns = m_possibleTurns;
      return;
    }

    outgoingTurns.candidates.emplace_back(0.0, Segment(), ftypes::HighwayClass::Tertiary, false);
    outgoingTurns.isCandidatesAngleValid = false;
  }

  double GetPathLength() const override
  {
    NOTIMPLEMENTED();
    return 0.0;
  }

  geometry::PointWithAltitude GetStartPoint() const override
  {
    NOTIMPLEMENTED();
    return geometry::PointWithAltitude();
  }

  geometry::PointWithAltitude GetEndPoint() const override
  {
    NOTIMPLEMENTED();
    return geometry::PointWithAltitude();
  }

private:
  TUnpackedPathSegments m_segments;
  size_t m_ingoingCount = 0;
  TurnCandidates m_possibleTurns;
  bool m_hasPossibleTurns = false;
};

UNIT_TEST(TestFixupTurns)
{
  double const kHalfSquareSideMeters = 10.;
  m2::PointD const kSquareCenterLonLat = {0., 0.};
  m2::RectD const kSquareNearZero =
      mercator::MetersToXY(kSquareCenterLonLat.x, kSquareCenterLonLat.y, kHalfSquareSideMeters);
  {
    // Removing a turn in case staying on a roundabout.
    vector<m2::PointD> const pointsMerc1 = {{kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareNearZero.maxX(), kSquareNearZero.maxY()},
                                            {kSquareNearZero.maxX(), kSquareNearZero.minY()}};
    // The constructor TurnItem(uint32_t idx, CarDirection t, uint32_t exitNum = 0)
    // is used for initialization of vector<TurnItem> below.
    vector<turns::TurnItem> turnsDir1 = {
        {1, CarDirection::EnterRoundAbout}, {2, CarDirection::StayOnRoundAbout}, {3, CarDirection::LeaveRoundAbout}};
    vector<RouteSegment> routeSegments;
    RouteSegmentsFrom({}, pointsMerc1, turnsDir1, {}, routeSegments);
    FixupCarTurns(routeSegments);
    vector<turns::TurnItem> const expectedTurnDir1 = {
        {1, CarDirection::EnterRoundAbout, 2}, {2, CarDirection::None, 0}, {3, CarDirection::LeaveRoundAbout, 2}};
    TEST_EQUAL(routeSegments[0].GetTurn(), expectedTurnDir1[0], ());
    TEST_EQUAL(routeSegments[1].GetTurn(), expectedTurnDir1[1], ());
    TEST_EQUAL(routeSegments[2].GetTurn(), expectedTurnDir1[2], ());
  }
  {
    // Merging turns which are close to each other.
    vector<m2::PointD> const pointsMerc2 = {{kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareNearZero.minX(), kSquareNearZero.minY()},
                                            {kSquareCenterLonLat.x, kSquareCenterLonLat.y},
                                            {kSquareNearZero.maxX(), kSquareNearZero.maxY()}};
    vector<turns::TurnItem> turnsDir2 = {
        {1, CarDirection::None}, {2, CarDirection::GoStraight}, {3, CarDirection::TurnLeft}};
    vector<RouteSegment> routeSegments2;
    RouteSegmentsFrom({}, pointsMerc2, turnsDir2, {}, routeSegments2);
    FixupCarTurns(routeSegments2);
    vector<turns::TurnItem> const expectedTurnDir2 = {
        {1, CarDirection::None}, {2, CarDirection::None}, {3, CarDirection::TurnLeft}};
    TEST_EQUAL(routeSegments2[0].GetTurn(), expectedTurnDir2[0], ());
    TEST_EQUAL(routeSegments2[1].GetTurn(), expectedTurnDir2[1], ());
    TEST_EQUAL(routeSegments2[2].GetTurn(), expectedTurnDir2[2], ());
  }
  {
    // No turn is removed.
    vector<m2::PointD> const pointsMerc3 = {
        {kSquareNearZero.minX(), kSquareNearZero.minY()},
        {kSquareNearZero.minX(), kSquareNearZero.maxY()},
        {kSquareNearZero.maxX(), kSquareNearZero.maxY()},
    };
    vector<turns::TurnItem> turnsDir3 = {{1, CarDirection::None}, {2, CarDirection::TurnRight}};

    vector<RouteSegment> routeSegments3;
    RouteSegmentsFrom({}, {}, turnsDir3, {}, routeSegments3);
    FixupCarTurns(routeSegments3);
    vector<turns::TurnItem> const expectedTurnDir3 = {{1, CarDirection::None}, {2, CarDirection::TurnRight}};

    TEST_EQUAL(routeSegments3[0].GetTurn(), expectedTurnDir3[0], ());
    TEST_EQUAL(routeSegments3[1].GetTurn(), expectedTurnDir3[1], ());
  }
}

UNIT_TEST(TestGetRoundaboutDirection)
{
  // The signature of GetRoundaboutDirection function is
  // GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
  //     bool isMultiTurnJunction, bool keepTurnByHighwayClass)
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, true, true), CarDirection::StayOnRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, true, false), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, false, true), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, true, false, false), CarDirection::None, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(false, true, false, true), CarDirection::EnterRoundAbout, ());
  TEST_EQUAL(GetRoundaboutDirectionBasic(true, false, false, false), CarDirection::LeaveRoundAbout, ());
}

UNIT_TEST(TestInvertDirection)
{
  TEST_EQUAL(InvertDirection(CarDirection::TurnSlightRight), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnRight), CarDirection::TurnLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSharpRight), CarDirection::TurnSharpLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSlightLeft), CarDirection::TurnSlightRight, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSlightRight), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnLeft), CarDirection::TurnRight, ());
  TEST_EQUAL(InvertDirection(CarDirection::TurnSharpLeft), CarDirection::TurnSharpRight, ());
}

UNIT_TEST(TestRightmostDirection)
{
  TEST_EQUAL(RightmostDirection(180.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(RightmostDirection(170.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(RightmostDirection(90.), CarDirection::TurnRight, ());
  TEST_EQUAL(RightmostDirection(45.), CarDirection::TurnSlightRight, ());
  TEST_EQUAL(RightmostDirection(0.), CarDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-20.), CarDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-90.), CarDirection::GoStraight, ());
  TEST_EQUAL(RightmostDirection(-170.), CarDirection::GoStraight, ());
}

UNIT_TEST(TestLeftmostDirection)
{
  TEST_EQUAL(LeftmostDirection(180.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(170.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(90.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(45.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(0.), CarDirection::GoStraight, ());
  TEST_EQUAL(LeftmostDirection(-20.), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(LeftmostDirection(-90.), CarDirection::TurnLeft, ());
  TEST_EQUAL(LeftmostDirection(-170.), CarDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestIntermediateDirection)
{
  TEST_EQUAL(IntermediateDirection(180.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(IntermediateDirection(170.), CarDirection::TurnSharpRight, ());
  TEST_EQUAL(IntermediateDirection(90.), CarDirection::TurnRight, ());
  TEST_EQUAL(IntermediateDirection(45.), CarDirection::TurnSlightRight, ());
  TEST_EQUAL(IntermediateDirection(0.), CarDirection::GoStraight, ());
  TEST_EQUAL(IntermediateDirection(-20.), CarDirection::TurnSlightLeft, ());
  TEST_EQUAL(IntermediateDirection(-90.), CarDirection::TurnLeft, ());
  TEST_EQUAL(IntermediateDirection(-170.), CarDirection::TurnSharpLeft, ());
}

UNIT_TEST(TestCheckUTurnOnRoute)
{
  TUnpackedPathSegments pathSegments(4, LoadedPathSegment());
  pathSegments[0].m_roadNameInfo = {"A road", "", "", "", "", false};
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Trunk;
  pathSegments[0].m_onRoundabout = false;
  pathSegments[0].m_isLink = false;
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};
  pathSegments[0].m_segmentRange =
      SegmentRange(FeatureID(), 0 /* start seg id */, 1 /* end seg id */, true /* forward */,
                   pathSegments[0].m_path.front().GetPoint(), pathSegments[0].m_path.back().GetPoint());

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_segmentRange =
      SegmentRange(FeatureID(), 1 /* start seg id */, 2 /* end seg id */, true /* forward */,
                   pathSegments[1].m_path.front().GetPoint(), pathSegments[1].m_path.back().GetPoint());
  pathSegments[1].m_path = {{{0, 1}, 0}, {{0, 0}, 0}};

  pathSegments[2] = pathSegments[0];
  pathSegments[2].m_segmentRange =
      SegmentRange(FeatureID(), 2 /* start seg id */, 3 /* end seg id */, true /* forward */,
                   pathSegments[2].m_path.front().GetPoint(), pathSegments[2].m_path.back().GetPoint());
  pathSegments[2].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};

  pathSegments[3] = pathSegments[0];
  pathSegments[3].m_segmentRange =
      SegmentRange(FeatureID(), 3 /* start seg id */, 4 /* end seg id */, true /* forward */,
                   pathSegments[3].m_path.front().GetPoint(), pathSegments[3].m_path.back().GetPoint());
  pathSegments[3].m_path.clear();

  RoutingResultTest resultTest(pathSegments);
  RoutingSettings const vehicleSettings = GetRoutingSettings(VehicleType::Car);
  // Zigzag test.
  TurnItem turn1;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings, turn1), 1, ());
  TEST_EQUAL(turn1.m_turn, CarDirection::UTurnLeft, ());
  TurnItem turn2;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 2 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings, turn2), 1, ());
  TEST_EQUAL(turn2.m_turn, CarDirection::UTurnLeft, ());

  // Empty path test.
  TurnItem turn3;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 3 /* outgoingSegmentIndex */, NumMwmIds(), vehicleSettings, turn3), 0, ());
}

UNIT_TEST(CheckUTurnOnRoute_ContinuousHairpin)
{
  NumMwmId constexpr kMwmId = 0;
  uint32_t constexpr kFeatureId = 1;
  uint32_t constexpr kIngoingSegmentIdx = 10;

  auto const TestContinuousHairpin = [&](bool const forward)
  {
    TUnpackedPathSegments pathSegments(3, LoadedPathSegment());
    pathSegments[0].m_roadNameInfo = {"A road", "", "", "", "", false};
    pathSegments[0].m_highwayClass = ftypes::HighwayClass::Trunk;
    pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};

    pathSegments[1] = pathSegments[0];
    pathSegments[1].m_path = {{{0, 1}, 0}, {{0.2, 0}, 0}};

    if (forward)
    {
      pathSegments[0].m_segments.emplace_back(kMwmId, kFeatureId, kIngoingSegmentIdx, true /* forward */);
      pathSegments[1].m_segments.emplace_back(kMwmId, kFeatureId, kIngoingSegmentIdx + 1, true /* forward */);
    }
    else
    {
      pathSegments[0].m_segments.emplace_back(kMwmId, kFeatureId, kIngoingSegmentIdx + 1, false /* forward */);
      pathSegments[1].m_segments.emplace_back(kMwmId, kFeatureId, kIngoingSegmentIdx, false /* forward */);
    }

    RoutingResultTest resultTest(pathSegments);
    TurnItem turn;
    TEST_EQUAL(CheckUTurnOnRoute(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(),
                                 GetRoutingSettings(VehicleType::Car), turn),
               0, (forward));
    TEST_EQUAL(turn.m_turn, CarDirection::None, (forward));
  };

  TestContinuousHairpin(true /* forward */);
  TestContinuousHairpin(false /* forward */);
}

UNIT_TEST(CheckUTurnOnRoute_WideUTurnOnDifferentFeatures)
{
  NumMwmId constexpr kMwmId = 0;

  TUnpackedPathSegments pathSegments(3, LoadedPathSegment());
  pathSegments[0].m_roadNameInfo = {"A road", "", "", "", "", false};
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Trunk;
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};
  pathSegments[0].m_segments.emplace_back(kMwmId, 1 /* featureId */, 10 /* segmentIdx */, true /* forward */);

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_path = {{{0, 1}, 0}, {{0.2, 0}, 0}};
  pathSegments[1].m_segments.clear();
  pathSegments[1].m_segments.emplace_back(kMwmId, 2 /* featureId */, 0 /* segmentIdx */, true /* forward */);

  RoutingResultTest resultTest(pathSegments);
  TurnItem turn;
  TEST_EQUAL(CheckUTurnOnRoute(resultTest, 1 /* outgoingSegmentIndex */, NumMwmIds(),
                               GetRoutingSettings(VehicleType::Car), turn),
             1, ());
  TEST_EQUAL(turn.m_turn, CarDirection::UTurnRight, ());
}

UNIT_TEST(GetTurnDirectionBasic_ContinuousHairpin)
{
  NumMwmId constexpr kMwmId = 0;
  uint32_t constexpr kFeatureId = 1;
  uint32_t constexpr kIngoingSegmentIdx = 10;

  FeatureID const featureId = routing_test::MakeTestFeatureID(kFeatureId);
  TUnpackedPathSegments pathSegments(2, LoadedPathSegment());
  // SP246 has no name and is identified by its ref only.
  pathSegments[0].m_roadNameInfo = {"", "IT:VI/SP246", "", "", "", false};
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Primary;
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}};
  pathSegments[0].m_segmentRange =
      SegmentRange(featureId, kIngoingSegmentIdx, kIngoingSegmentIdx, true /* forward */,
                   pathSegments[0].m_path.front().GetPoint(), pathSegments[0].m_path.back().GetPoint());
  pathSegments[0].m_segments.emplace_back(kMwmId, kFeatureId, kIngoingSegmentIdx, true /* forward */);

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_path = {{{0, 1}, 0}, {{-0.2, 0}, 0}};
  pathSegments[1].m_segmentRange =
      SegmentRange(featureId, kIngoingSegmentIdx + 1, kIngoingSegmentIdx + 1, true /* forward */,
                   pathSegments[1].m_path.front().GetPoint(), pathSegments[1].m_path.back().GetPoint());
  pathSegments[1].m_segments.clear();
  pathSegments[1].m_segments.emplace_back(kMwmId, kFeatureId, kIngoingSegmentIdx + 1, true /* forward */);

  NumMwmIds numMwmIds;
  numMwmIds.RegisterFile(platform::CountryFile("0"));

  auto const TestStraightCandidate = [&](ftypes::HighwayClass const candidateClass,
                                         CarDirection const expectedDirection)
  {
    RoutingResultTest resultTest(pathSegments);
    TurnCandidates turns;
    turns.isCandidatesAngleValid = true;
    double const routeAngle = CalcOneSegmentTurnAngle(TurnInfo(&pathSegments[0], &pathSegments[1]));
    turns.candidates.emplace_back(routeAngle,
                                  Segment(kMwmId, kFeatureId, kIngoingSegmentIdx + 1, true /* forward */),
                                  ftypes::HighwayClass::Primary, false /* isLink */);
    turns.candidates.emplace_back(0.0, Segment(kMwmId, 2 /* featureId */, 0 /* segmentIdx */, true /* forward */),
                                  candidateClass, false /* isLink */);
    resultTest.SetPossibleTurns(2 /* ingoingCount */, turns);

    TurnItem turn;
    CarDirection prevDir = CarDirection::None;
    GetTurnDirectionBasic(resultTest, 1 /* outgoingSegmentIndex */, numMwmIds, GetRoutingSettings(VehicleType::Car),
                          turn, prevDir);
    TEST_EQUAL(turn.m_turn, expectedDirection, (candidateClass));
  };

  TestStraightCandidate(ftypes::HighwayClass::Service, CarDirection::None);
  TestStraightCandidate(ftypes::HighwayClass::Primary, CarDirection::TurnSharpLeft);
}

UNIT_TEST(GetNextRoutePointIndex)
{
  TUnpackedPathSegments pathSegments(2, LoadedPathSegment());
  pathSegments[0].m_path = {{{0, 0}, 0}, {{0, 1}, 0}, {{0, 2}, 0}};
  pathSegments[1].m_path = {{{0, 2}, 0}, {{1, 2}, 0}};

  RoutingResultTest resultTest(pathSegments);
  RoutePointIndex nextIndex;

  // Forward direction.
  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), NumMwmIds(),
                              true /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                              true /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), ());

  // Trying to get next item after the last item of the first segment.
  // False because of too sharp turn angle.
  TEST(!GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), NumMwmIds(),
                               true /* forward */, nextIndex),
       ());

  // Trying to get point about the end of the route.
  TEST(!GetNextRoutePointIndex(resultTest, RoutePointIndex({1 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                               true /* forward */, nextIndex),
       ());

  // Backward direction.
  // Moving in backward direction it's possible to get index of the first item of a segment.
  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({1 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                              false /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({1 /* m_segmentIndex */, 0 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 2 /* m_pathIndex */}), NumMwmIds(),
                              false /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), ());

  TEST(GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 1 /* m_pathIndex */}), NumMwmIds(),
                              false /* forward */, nextIndex),
       ());
  TEST_EQUAL(nextIndex, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), ());

  // Trying to get point before the beginning.
  TEST(!GetNextRoutePointIndex(resultTest, RoutePointIndex({0 /* m_segmentIndex */, 0 /* m_pathIndex */}), NumMwmIds(),
                               false /* forward */, nextIndex),
       ());
}

UNIT_TEST(TestTurnAcrossCarriageway)
{
  /*
    This test models a two-carriageway road "Hyde Drive",
    with a one-carriageway road "Pine street" intersecting it as follows.

      Hyde Drive
        ↓    ↑
        ↓    ↑
    ----+====x=== Pine street
        ↓    ↑
        ↓    ↑

    We test that, when coming from south and turning left into Pine street at the "x",
    no "go straight" instruction is generated at "+", even if the highwayClass becomes smaller.
   */

  NumMwmId kMwmId = 0;
  uint32_t kHydeDriveNorthFeatureId = 1;
  uint32_t kHydeDriveSouthFeatureId = 2;
  uint32_t kPineStreetFeatureId = 3;

  FeatureID hydeDriveNorthFeature = routing_test::MakeTestFeatureID(kHydeDriveNorthFeatureId);
  FeatureID hydeDriveSouthFeature = routing_test::MakeTestFeatureID(kHydeDriveSouthFeatureId);
  FeatureID pineStreetFeature = routing_test::MakeTestFeatureID(kPineStreetFeatureId);

  NumMwmIds numMwmIds;
  numMwmIds.RegisterFile(platform::CountryFile("0"));

  auto P = [](double x, double y)
  { return geometry::PointWithAltitude({mercator::MetersToMercator(x), mercator::MetersToMercator(y)}, 0); };

  TUnpackedPathSegments pathSegments(7, LoadedPathSegment());
  pathSegments[0].m_isLink = false;
  pathSegments[0].m_onRoundabout = false;
  pathSegments[0].m_isOneWay = true;
  pathSegments[0].m_roadNameInfo = {"Hyde Drive", "", "", "", "", false};
  pathSegments[0].m_highwayClass = ftypes::HighwayClass::Primary;
  pathSegments[0].m_path = {P(20, 0), P(20, 10)};
  pathSegments[0].m_segmentRange =
      SegmentRange(hydeDriveNorthFeature, 0, 0, true /* forward */, pathSegments[0].m_path.front().GetPoint(),
                   pathSegments[0].m_path.back().GetPoint());
  pathSegments[0].m_segments.emplace_back(kMwmId, kHydeDriveNorthFeatureId, 0, true /* forward */);

  pathSegments[1] = pathSegments[0];
  pathSegments[1].m_path = {P(20, 10), P(20, 20)};
  pathSegments[1].m_segmentRange =
      SegmentRange(hydeDriveNorthFeature, 1, 1, true /* forward */, pathSegments[1].m_path.front().GetPoint(),
                   pathSegments[1].m_path.back().GetPoint());
  pathSegments[1].m_segments = {Segment(kMwmId, kHydeDriveNorthFeatureId, 1, true /* forward */)};

  pathSegments[2] = pathSegments[1];
  pathSegments[2].m_path = {P(10, 20), P(10, 10)};
  pathSegments[2].m_segmentRange =
      SegmentRange(hydeDriveSouthFeature, 0, 0, true /* forward */, pathSegments[2].m_path.front().GetPoint(),
                   pathSegments[2].m_path.back().GetPoint());
  pathSegments[2].m_segments = {Segment(kMwmId, kHydeDriveSouthFeatureId, 0, true /* forward */)};

  pathSegments[3] = pathSegments[2];
  pathSegments[3].m_path = {P(10, 10), P(10, 0)};
  pathSegments[3].m_segmentRange =
      SegmentRange(hydeDriveSouthFeature, 1, 1, true /* forward */, pathSegments[3].m_path.front().GetPoint(),
                   pathSegments[3].m_path.back().GetPoint());
  pathSegments[3].m_segments = {Segment(kMwmId, kHydeDriveSouthFeatureId, 1, true /* forward */)};

  pathSegments[4].m_isLink = false;
  pathSegments[4].m_onRoundabout = false;
  pathSegments[4].m_isOneWay = false;
  pathSegments[4].m_roadNameInfo = {"Pine street", "", "", "", "", false};
  pathSegments[4].m_highwayClass = ftypes::HighwayClass::Tertiary;
  pathSegments[4].m_path = {P(30, 10), P(20, 10)};
  pathSegments[4].m_segmentRange =
      SegmentRange(pineStreetFeature, 0, 0, true /* forward */, pathSegments[4].m_path.front().GetPoint(),
                   pathSegments[4].m_path.back().GetPoint());
  pathSegments[4].m_segments.emplace_back(kMwmId, kPineStreetFeatureId, 0, true /* forward */);

  pathSegments[5] = pathSegments[4];
  pathSegments[5].m_highwayClass = ftypes::HighwayClass::Primary;
  pathSegments[5].m_path = {P(20, 10), P(10, 10)};
  pathSegments[5].m_segmentRange =
      SegmentRange(pineStreetFeature, 1, 1, true /* forward */, pathSegments[5].m_path.front().GetPoint(),
                   pathSegments[5].m_path.back().GetPoint());
  pathSegments[5].m_segments = {Segment(kMwmId, kPineStreetFeatureId, 1, true /* forward */)};

  pathSegments[6] = pathSegments[5];
  pathSegments[6].m_highwayClass = ftypes::HighwayClass::Tertiary;
  pathSegments[6].m_path = {P(10, 10), P(0, 10)};
  pathSegments[6].m_segmentRange =
      SegmentRange(pineStreetFeature, 2, 2, true /* forward */, pathSegments[6].m_path.front().GetPoint(),
                   pathSegments[6].m_path.back().GetPoint());
  pathSegments[6].m_segments = {Segment(kMwmId, kPineStreetFeatureId, 2, true /* forward */)};

  // The route comes from south on Hyde Drive (0), turns left into Pine street (5),
  // and continues straight west (6).
  TUnpackedPathSegments route = {pathSegments[0], pathSegments[5], pathSegments[6]};
  RoutingResultTest resultTest(route);

  RoutingSettings const vehicleSettings = GetRoutingSettings(VehicleType::Car);
  CarDirection previousDirection = CarDirection::None;

  TurnItem turnAtX;
  TurnCandidates turnsAtX;
  double routeAngleAtX = CalcOneSegmentTurnAngle(TurnInfo(&route[0], &route[1]));
  turnsAtX.isCandidatesAngleValid = true;
  turnsAtX.candidates.emplace_back(routeAngleAtX, pathSegments[5].m_segments.front(), pathSegments[5].m_highwayClass,
                                   false /* isLink */);
  turnsAtX.candidates.emplace_back(0.0, pathSegments[1].m_segments.front(), ftypes::HighwayClass::Primary,
                                   false /* isLink */);
  resultTest.SetPossibleTurns(2 /* ingoingCount */, turnsAtX);
  GetTurnDirectionBasic(resultTest, 1 /* outgoingSegmentIndex */, numMwmIds, vehicleSettings, turnAtX,
                        previousDirection);
  TEST_EQUAL(turnAtX.m_turn, CarDirection::TurnLeft, ());
  TEST_EQUAL(previousDirection, CarDirection::TurnLeft, ());

  TurnItem turnAtPlus;
  TurnCandidates turnsAtPlus;
  double routeAngleAtPlus = CalcOneSegmentTurnAngle(TurnInfo(&route[1], &route[2]));
  turnsAtPlus.isCandidatesAngleValid = true;
  turnsAtPlus.candidates.emplace_back(-90.0, pathSegments[3].m_segments.front(), ftypes::HighwayClass::Primary,
                                      false /* isLink */);
  turnsAtPlus.candidates.emplace_back(routeAngleAtPlus, pathSegments[6].m_segments.front(),
                                      ftypes::HighwayClass::Tertiary, false /* isLink */);
  resultTest.SetPossibleTurns(2 /* ingoingCount */, turnsAtPlus);
  GetTurnDirectionBasic(resultTest, 2 /* outgoingSegmentIndex */, numMwmIds, vehicleSettings, turnAtPlus,
                        previousDirection);
  TEST_EQUAL(turnAtPlus.m_turn, CarDirection::None, ());

  // Verify that without the prior turn across the dual carriageway,
  // a "GoStraight" would normally be generated when entering a lower road class across a higher class.
  CarDirection noPriorTurn = CarDirection::None;
  TurnItem turnWithoutPriorTurn;
  GetTurnDirectionBasic(resultTest, 2 /* outgoingSegmentIndex */, numMwmIds, vehicleSettings, turnWithoutPriorTurn,
                        noPriorTurn);
  TEST_EQUAL(turnWithoutPriorTurn.m_turn, CarDirection::GoStraight, ());
}
}  // namespace turn_generator_test
