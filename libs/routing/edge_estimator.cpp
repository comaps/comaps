#include "routing/edge_estimator.hpp"

#include "routing/geometry.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/segment.hpp"
#include "routing/traffic_stash.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace routing
{
using namespace std;
using namespace traffic;
using measurement_utils::KmphToMps;

namespace
{
geometry::Altitude constexpr kMountainSicknessAltitudeM = 2500;

double TimeBetweenSec(ms::LatLon const & from, ms::LatLon const & to, double speedMpS)
{
  ASSERT_GREATER(speedMpS, 0.0, ("from:", from, "to:", to));

  double const distanceM = ms::DistanceOnEarth(from, to);
  return distanceM / speedMpS;
}

double CalcTrafficFactor(SpeedGroup speedGroup)
{
  if (speedGroup == SpeedGroup::TempBlock)
  {
    // impossible driving factor
    return 1.0E4;
  }

  double const percentage = 0.01 * kSpeedGroupThresholdPercentage[static_cast<size_t>(speedGroup)];
  ASSERT_GREATER(percentage, 0.0, (speedGroup));
  return 1.0 / percentage;
}

double GetSpeedMpS(EdgeEstimator::Purpose purpose, Segment const & segment, RoadGeometry const & road)
{
  SpeedKMpH const & speed = road.GetSpeed(segment.IsForward());
  double const speedMpS = KmphToMps(purpose == EdgeEstimator::Purpose::Weight ? speed.m_weight : speed.m_eta);
  ASSERT_GREATER(speedMpS, 0.0, (segment));
  return speedMpS;
}

bool IsTransit(std::optional<HighwayType> type)
{
  return type && (type == HighwayType::RouteFerry || type == HighwayType::RouteShuttleTrain);
}

template <class CalcSpeed>
double CalcClimbSegment(EdgeEstimator::Purpose purpose, Segment const & segment, RoadGeometry const & road,
                        CalcSpeed && calcSpeed)
{
  double const distance = road.GetDistance(segment.GetSegmentIdx());
  double speedMpS = GetSpeedMpS(purpose, segment, road);

  static double constexpr kSmallDistanceM = 1;  // we have altitude threshold is 0.5m
  if (distance > kSmallDistanceM && !IsTransit(road.GetHighwayType()))
  {
    LatLonWithAltitude const & from = road.GetJunction(segment.GetPointId(false /* front */));
    LatLonWithAltitude const & to = road.GetJunction(segment.GetPointId(true /* front */));

    ASSERT(to.GetAltitude() != geometry::kInvalidAltitude && from.GetAltitude() != geometry::kInvalidAltitude, ());
    auto const altitudeDiff = to.GetAltitude() - from.GetAltitude();

    if (altitudeDiff != 0)
    {
      speedMpS = calcSpeed(speedMpS, altitudeDiff / distance, to.GetAltitude());
      ASSERT_GREATER(speedMpS, 0.0, (segment));
    }
  }

  return distance / speedMpS;
}
}  // namespace

double GetPedestrianClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM)
{
  double constexpr kMinPenalty = 1.0;
  // Descent penalty is less then the ascent penalty.
  double const impact = tangent >= 0.0 ? 1.0 : 0.35;

  if (altitudeM >= kMountainSicknessAltitudeM)
    return kMinPenalty + (10.0 + (altitudeM - kMountainSicknessAltitudeM) * 10.0 / 1500.0) * fabs(tangent) * impact;

  // Use magic constant from this table: https://en.wikipedia.org/wiki/Tobler's_hiking_function#Sample_values
  // Tobler's returns unusually big values for bigger tangent.
  // See Australia_Mountains_Downlhill test.
  if (purpose == EdgeEstimator::Purpose::Weight || fabs(tangent) > 1.19)
  {
    tangent = fabs(tangent);
    // Some thoughts about gradient and foot walking: https://gre-kow.livejournal.com/26916.html
    // 3cm diff with avg foot length 60cm is imperceptible (see Hungary_UseFootways).
    double constexpr kTangentThreshold = 3.0 / 60.0;
    if (tangent < kTangentThreshold)
      return kMinPenalty;

    // ETA coefficients are calculated in https://github.com/mapsme/omim-scripts/pull/21
    auto const penalty = purpose == EdgeEstimator::Purpose::Weight ? 5.0 * tangent + 7.0 * tangent * tangent
                                                                   : 3.01 * tangent + 3.54 * tangent * tangent;

    return kMinPenalty + penalty * impact;
  }
  else
  {
    // Use Tobler’s Hiking Function for ETA like more comprehensive. See France_Uphill_Downlhill test.
    // Why not in Weight? See Crimea_Altitude_Mountains test.
    // https://mtntactical.com/research/yet-calculating-movement-uneven-terrain/
    // Returns factor: W(0) / W(tangent).
    return exp(-3.5 * (0.05 - fabs(tangent + 0.05)));
  }
}

double GetBicycleClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM)
{
  double constexpr kMinPenalty = 1.0;
  double const impact = tangent >= 0.0 ? 1.0 : 0.35;

  if (altitudeM >= kMountainSicknessAltitudeM)
    return kMinPenalty + 50.0 * fabs(tangent) * impact;

  // By VNG: This approach is strange at least because it always returns penalty > 1 (even for downhill)
  /*
  tangent = fabs(tangent);
  // ETA coefficients are calculated in https://github.com/mapsme/omim-scripts/pull/22
  auto const penalty = purpose == EdgeEstimator::Purpose::Weight
                           ? 10.0 * tangent + 26.0 * tangent * tangent
                           : 8.8 * tangent + 6.51 * tangent * tangent;

  return kMinPenalty + penalty * impact;
  */

  // https://web.tecnico.ulisboa.pt/~rosamfelix/gis/declives/SpeedSlopeFactor.html
  double const slope = tangent * 100;

  double factor;
  if (slope < -30)
    factor = 1.5;
  else if (slope < 0)
  {
    // Min factor (max speed) will be at slope = -13.
    factor = 1 + 2 * 0.7 / 13.0 * slope + 0.7 / 169 * slope * slope;
  }
  else if (slope <= 20)
    factor = 1 + slope * slope / 49;
  else
    factor = 10.0;
  return factor;
}

// EdgeEstimator -----------------------------------------------------------------------------------
EdgeEstimator::EdgeEstimator(VehicleType vehicleType, double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH,
                             DataSource * /*dataSourcePtr*/, std::shared_ptr<NumMwmIds> /*numMwmIds*/)
  : m_vehicleType(vehicleType)
  , m_maxWeightSpeedMpS(KmphToMps(maxWeightSpeedKMpH))
  , m_offroadSpeedKMpH(offroadSpeedKMpH)

//, m_dataSourcePtr(dataSourcePtr)
//, m_numMwmIds(numMwmIds)
{
  CHECK_GREATER(m_offroadSpeedKMpH.m_weight, 0.0, ());
  CHECK_GREATER(m_offroadSpeedKMpH.m_eta, 0.0, ());
  CHECK_GREATER_OR_EQUAL(m_maxWeightSpeedMpS, KmphToMps(m_offroadSpeedKMpH.m_weight), ());

  if (m_offroadSpeedKMpH.m_eta != kNotUsed)
    CHECK_GREATER_OR_EQUAL(m_maxWeightSpeedMpS, KmphToMps(m_offroadSpeedKMpH.m_eta), ());

  LOG(LINFO, ("Loading turn penalties for vehicle type:", static_cast<int>(vehicleType)));

  struct TurnPenaltyMatrix
  {
    int road;
    VehicleType vehicleType;
    double penalty;
  };

  struct TurnPenalty
  {
    HighwayType fromRoadType;
    HighwayType toRoadType;
    VehicleType vehicleType;
    double penalty;
  };

#define N 288

  static auto constexpr kTurnPenaltyMatrix = []
  {
    array<TurnPenalty, N> constexpr kTable = {{
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayPrimary, VehicleType::Car, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayResidential, VehicleType::Car, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwaySecondary, VehicleType::Car, 0.08},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayService, VehicleType::Car, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTrunk, VehicleType::Car, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayUnclassified, VehicleType::Car, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.11},
        {HighwayType::HighwayPrimary, HighwayType::HighwayPrimary, VehicleType::Car, 0.06},
        {HighwayType::HighwayPrimary, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.06},
        {HighwayType::HighwayPrimary, HighwayType::HighwayResidential, VehicleType::Car, 0.1},
        {HighwayType::HighwayPrimary, HighwayType::HighwaySecondary, VehicleType::Car, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwayService, VehicleType::Car, 0.1},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTertiary, VehicleType::Car, 0.08},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTrunk, VehicleType::Car, 0.04},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.04},
        {HighwayType::HighwayPrimary, HighwayType::HighwayUnclassified, VehicleType::Car, 0.1},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.1},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayPrimary, VehicleType::Car, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayResidential, VehicleType::Car, 0.1},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwaySecondary, VehicleType::Car, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayService, VehicleType::Car, 0.09},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTertiary, VehicleType::Car, 0.08},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTrunk, VehicleType::Car, 0.07},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayUnclassified, VehicleType::Car, 0.1},
        {HighwayType::HighwayResidential, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayPrimary, VehicleType::Car, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayResidential, VehicleType::Car, 0.08},
        {HighwayType::HighwayResidential, HighwayType::HighwaySecondary, VehicleType::Car, 0.08},
        {HighwayType::HighwayResidential, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwayResidential, HighwayType::HighwayService, VehicleType::Car, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayTrunk, VehicleType::Car, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayUnclassified, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.1},
        {HighwayType::HighwaySecondary, HighwayType::HighwayPrimary, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayResidential, VehicleType::Car, 0.1},
        {HighwayType::HighwaySecondary, HighwayType::HighwaySecondary, VehicleType::Car, 0.08},
        {HighwayType::HighwaySecondary, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwaySecondary, HighwayType::HighwayService, VehicleType::Car, 0.08},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTrunk, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.06},
        {HighwayType::HighwaySecondary, HighwayType::HighwayUnclassified, VehicleType::Car, 0.08},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayPrimary, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayResidential, VehicleType::Car, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwaySecondary, VehicleType::Car, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayService, VehicleType::Car, 0.06},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTertiary, VehicleType::Car, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTrunk, VehicleType::Car, 0.08},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.08},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayUnclassified, VehicleType::Car, 0.06},
        {HighwayType::HighwayService, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayPrimary, VehicleType::Car, 0.09},
        {HighwayType::HighwayService, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayService, HighwayType::HighwayResidential, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwaySecondary, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayService, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTrunk, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayUnclassified, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.08},
        {HighwayType::HighwayTertiary, HighwayType::HighwayPrimary, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayResidential, VehicleType::Car, 0.08},
        {HighwayType::HighwayTertiary, HighwayType::HighwaySecondary, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayService, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTrunk, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayUnclassified, VehicleType::Car, 0.08},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayPrimary, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayResidential, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwaySecondary, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayService, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTrunk, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayUnclassified, VehicleType::Car, 0.07},
        {HighwayType::HighwayTrunk, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.1},
        {HighwayType::HighwayTrunk, HighwayType::HighwayPrimary, VehicleType::Car, 0.05},
        {HighwayType::HighwayTrunk, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.03},
        {HighwayType::HighwayTrunk, HighwayType::HighwayResidential, VehicleType::Car, 0.09},
        {HighwayType::HighwayTrunk, HighwayType::HighwaySecondary, VehicleType::Car, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayService, VehicleType::Car, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTertiary, VehicleType::Car, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTrunk, VehicleType::Car, 0.01},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.01},
        {HighwayType::HighwayTrunk, HighwayType::HighwayUnclassified, VehicleType::Car, 0.08},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.11},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayPrimary, VehicleType::Car, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayResidential, VehicleType::Car, 0.1},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwaySecondary, VehicleType::Car, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayService, VehicleType::Car, 0.07},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTertiary, VehicleType::Car, 0.06},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.06},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTrunk, VehicleType::Car, 0.07},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.02},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayUnclassified, VehicleType::Car, 0.1},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayLivingStreet, VehicleType::Car, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayPrimary, VehicleType::Car, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayPrimaryLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayResidential, VehicleType::Car, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwaySecondary, VehicleType::Car, 0.08},
        {HighwayType::HighwayUnclassified, HighwayType::HighwaySecondaryLink, VehicleType::Car, 0.08},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayService, VehicleType::Car, 0.08},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTertiary, VehicleType::Car, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTertiaryLink, VehicleType::Car, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTrunk, VehicleType::Car, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTrunkLink, VehicleType::Car, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayUnclassified, VehicleType::Car, 0.08},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayService, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayLivingStreet, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.11},
        {HighwayType::HighwayPrimary, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayPrimary, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayPrimary, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayPrimary, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwayService, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.04},
        {HighwayType::HighwayPrimary, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.04},
        {HighwayType::HighwayPrimary, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayService, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayPrimaryLink, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayResidential, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayResidential, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayResidential, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayResidential, HighwayType::HighwayService, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayResidential, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayResidential, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwaySecondary, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwaySecondary, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwaySecondary, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwaySecondary, HighwayType::HighwayService, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondary, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwaySecondary, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayService, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.05},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwaySecondaryLink, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayService, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayService, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayService, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayService, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayService, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTertiary, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTertiary, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayService, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiary, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayService, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTertiaryLink, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTrunk, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayTrunk, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.05},
        {HighwayType::HighwayTrunk, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.03},
        {HighwayType::HighwayTrunk, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayTrunk, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayService, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.01},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.01},
        {HighwayType::HighwayTrunk, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.11},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.04},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayService, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.06},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.02},
        {HighwayType::HighwayTrunkLink, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.1},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayLivingStreet, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayPrimary, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayPrimaryLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayResidential, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwaySecondary, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayUnclassified, HighwayType::HighwaySecondaryLink, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayService, VehicleType::Bicycle, 0.08},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTertiary, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTertiaryLink, VehicleType::Bicycle, 0.07},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTrunk, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayTrunkLink, VehicleType::Bicycle, 0.09},
        {HighwayType::HighwayUnclassified, HighwayType::HighwayUnclassified, VehicleType::Bicycle, 0.08},
    }};

    array<TurnPenaltyMatrix, N> result{};

    for (size_t i = 0; i < N; ++i)
      result[i] = {static_cast<int>(kTable[i].fromRoadType) * 65535 + static_cast<int>(kTable[i].toRoadType),
                   kTable[i].vehicleType, kTable[i].penalty};

    return result;
  }();

  double totalPenalty = 0;

  for (TurnPenaltyMatrix const & row : kTurnPenaltyMatrix)
  {
    if (row.vehicleType != vehicleType)
      continue;

    m_turnPenaltyMap[row.road] += row.penalty;
    totalPenalty += row.penalty;
  }

  if (!m_turnPenaltyMap.empty())
  {
    m_defaultPenalty = totalPenalty / m_turnPenaltyMap.size();
    LOG(LINFO, ("Loaded", m_turnPenaltyMap.size(), "turn penalties with default:", m_defaultPenalty));
  }
  else
  {
    LOG(LWARNING, ("No turn penalties loaded for vehicle type:", static_cast<int>(vehicleType)));
  }
}

double EdgeEstimator::CalcHeuristic(ms::LatLon const & from, ms::LatLon const & to) const
{
  // For the correct A*, we should use maximum _possible_ speed here, including:
  // default model, feature stored, unlimited autobahn, ferry or rail transit.
  return TimeBetweenSec(from, to, m_maxWeightSpeedMpS);
}

double EdgeEstimator::ComputeDefaultLeapWeightSpeed() const
{
  // 1.76 factor was computed as an average ratio of escape/enter speed to max MWM speed across all MWMs.
  // return m_maxWeightSpeedMpS / 1.76;

  // By VNG: Current m_maxWeightSpeedMpS is > 120 km/h, so estimating speed was > 60km/h
  // for start/end fake edges by straight line! I strongly believe that this is very! optimistic.
  // Set speed to 57.5km/h (16m/s):
  // - lower bound Russia_MoscowDesnogorsk (https://github.com/organicmaps/organicmaps/issues/1071)
  // - upper bound RussiaSmolenskRussiaMoscowTimeTest
  return 16.0;
}

/*
double EdgeEstimator::LoadLeapWeightSpeed(NumMwmId mwmId)
{
  double leapWeightSpeed = ComputeDefaultLeapWeightSpeed();

  if (m_dataSourcePtr)
  {
    MwmSet::MwmHandle handle =
        m_dataSourcePtr->GetMwmHandleByCountryFile(m_numMwmIds->GetFile(mwmId));
    if (!handle.IsAlive())
      MYTHROW(RoutingException, ("Mwm", m_numMwmIds->GetFile(mwmId), "cannot be loaded."));

    if (handle.GetInfo())
      leapWeightSpeed = handle.GetInfo()->GetRegionData().GetLeapWeightSpeed(leapWeightSpeed);
  }

  if (leapWeightSpeed > m_maxWeightSpeedMpS)
    leapWeightSpeed = m_maxWeightSpeedMpS;

  return leapWeightSpeed;
}
*/

double EdgeEstimator::GetLeapWeightSpeed(NumMwmId /*mwmId*/)
{
  double defaultSpeed = ComputeDefaultLeapWeightSpeed();

  /// @todo By VNG: We don't have LEAP_SPEEDS_FILE to assign RegionData::SetLeapWeightSpeed
  /// unique for each MWM, so this is useless now. And what about possible races here?
  //  if (mwmId != kFakeNumMwmId)
  //  {
  //    auto [speedIt, inserted] = m_leapWeightSpeedMpS.emplace(mwmId, defaultSpeed);
  //    if (inserted)
  //      speedIt->second = LoadLeapWeightSpeed(mwmId);
  //    return speedIt->second;
  //  }

  return defaultSpeed;
}

double EdgeEstimator::CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId)
{
  return TimeBetweenSec(from, to, GetLeapWeightSpeed(mwmId));
}

double EdgeEstimator::GetMaxWeightSpeedMpS() const
{
  return m_maxWeightSpeedMpS;
}

double EdgeEstimator::CalcOffroad(ms::LatLon const & from, ms::LatLon const & to, Purpose purpose) const
{
  auto const offroadSpeedKMpH = purpose == Purpose::Weight ? m_offroadSpeedKMpH.m_weight : m_offroadSpeedKMpH.m_eta;
  if (offroadSpeedKMpH == kNotUsed)
    return 0.0;

  return TimeBetweenSec(from, to, KmphToMps(offroadSpeedKMpH));
}

// PedestrianEstimator -----------------------------------------------------------------------------
class PedestrianEstimator final : public EdgeEstimator
{
public:
  PedestrianEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(VehicleType::Pedestrian, maxWeightSpeedKMpH, offroadSpeedKMpH)
  {}

  // EdgeEstimator overrides:
  double GetUTurnPenalty(Purpose /* purpose */) const override { return 0.0 /* seconds */; }

  double GetTurnPenalty(Purpose purpose, double angle, RoadGeometry const & from_road, RoadGeometry const & to_road,
                        bool is_left_hand_traffic = false) const override
  {
    return 0;
  }

  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 10 * 60;  // seconds
    case Purpose::ETA: return 8 * 60;      // seconds
    }
    UNREACHABLE();
  }

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const override
  {
    return CalcClimbSegment(purpose, segment, road,
                            [purpose](double speedMpS, double tangent, geometry::Altitude altitude)
    { return speedMpS / GetPedestrianClimbPenalty(purpose, tangent, altitude); });
  }
};

// BicycleEstimator --------------------------------------------------------------------------------
class BicycleEstimator final : public EdgeEstimator
{
public:
  BicycleEstimator(double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(VehicleType::Bicycle, maxWeightSpeedKMpH, offroadSpeedKMpH)
  {}

  // EdgeEstimator overrides:
  double GetUTurnPenalty(Purpose /* purpose */) const override { return 20.0 /* seconds */; }

  double GetTurnPenalty(Purpose purpose, double angle, RoadGeometry const & from_road, RoadGeometry const & to_road,
                        bool is_left_hand_traffic = false) const override;

  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 10 * 60;  // seconds
    case Purpose::ETA: return 8 * 60;      // seconds
    }
    UNREACHABLE();
  }

  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const override
  {
    return CalcClimbSegment(purpose, segment, road,
                            [purpose, this](double speedMpS, double tangent, geometry::Altitude altitude)
    {
      auto const factor = GetBicycleClimbPenalty(purpose, tangent, altitude);
      ASSERT_GREATER(factor, 0.0, ());

      /// @todo Take out "bad" bicycle road (path, track, footway, ...) check into BicycleModel?
      static double constexpr badBicycleRoadSpeed = KmphToMps(9);
      if (speedMpS <= badBicycleRoadSpeed)
      {
        if (factor > 1)
          speedMpS /= factor;
      }
      else if (factor > 1)
      {
        // Calculate uphill speed according to the average bicycle speed, because "good-roads" like
        // residential, secondary, cycleway are "equal-low-speed" uphill and road type doesn't matter.
        static double constexpr avgBicycleSpeed = KmphToMps(20);
        double const upperBound = avgBicycleSpeed / factor;
        if (speedMpS > upperBound)
        {
          // Add small weight to distinguish roads by class (10 is a max factor value).
          speedMpS = upperBound + (purpose == Purpose::Weight ? speedMpS / (10 * avgBicycleSpeed) : 0);
        }
      }
      else
        speedMpS /= factor;

      return std::min(speedMpS, GetMaxWeightSpeedMpS());
    });
  }
};

double BicycleEstimator::GetTurnPenalty(Purpose purpose, double angle, RoadGeometry const & from_road,
                                        RoadGeometry const & to_road, bool is_left_hand_traffic) const
{
  auto penalty = m_defaultPenalty;

  if (from_road.GetHighwayType().has_value() && to_road.GetHighwayType().has_value())
  {
    int const from_road_idx = static_cast<int>(from_road.GetHighwayType().value());
    int const to_road_idx = static_cast<int>(to_road.GetHighwayType().value());
    auto const pen = m_turnPenaltyMap.find(from_road_idx * 65535 + to_road_idx);
    if (pen != m_turnPenaltyMap.end())
      penalty = pen->second;
  }

  // Determine if turn crosses traffic based on driving side
  // For bicycles, crossing traffic is less dangerous than for cars but still a factor
  bool turn_crosses_traffic;
  if (is_left_hand_traffic)
    turn_crosses_traffic = angle < 0;
  else
    turn_crosses_traffic = angle > 0;

  // Penalty multiplier for bicycles crossing traffic
  auto const extra_penalty = turn_crosses_traffic ? 2.0 : 1.0;
  auto const result = fabs(angle) * penalty * extra_penalty;

  return result;
}

// CarEstimator ------------------------------------------------------------------------------------
class CarEstimator final : public EdgeEstimator
{
public:
  CarEstimator(DataSource * dataSourcePtr, std::shared_ptr<NumMwmIds> numMwmIds, shared_ptr<TrafficStash> trafficStash,
               double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH)
    : EdgeEstimator(VehicleType::Car, maxWeightSpeedKMpH, offroadSpeedKMpH, dataSourcePtr, numMwmIds)
    , m_trafficStash(std::move(trafficStash))
  {}

  // EdgeEstimator overrides:
  double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const override;
  double GetUTurnPenalty(Purpose /* purpose */) const override;
  double GetTurnPenalty(Purpose purpose, double angle, RoadGeometry const & from_road, RoadGeometry const & to_road,
                        bool is_left_hand_traffic = false) const override;
  double GetFerryLandingPenalty(Purpose purpose) const override
  {
    switch (purpose)
    {
    case Purpose::Weight: return 10 * 60;  // seconds
    case Purpose::ETA: return 10 * 60;     // seconds
    }
    UNREACHABLE();
  }

private:
  std::shared_ptr<TrafficStash> m_trafficStash;
};

double CarEstimator::GetUTurnPenalty(Purpose /* purpose */) const
{
  // Adds 2 minutes penalty for U-turn. The value is quite arbitrary
  // and needs to be properly selected after a number of real-world
  // experiments.
  return 2 * 60;  // seconds
}

double CarEstimator::GetTurnPenalty(Purpose purpose, double angle, RoadGeometry const & from_road,
                                    RoadGeometry const & to_road, bool is_left_hand_traffic) const
{
  auto penalty = m_defaultPenalty;

  if (from_road.GetHighwayType().has_value() && to_road.GetHighwayType().has_value())
  {
    int const from_road_idx = static_cast<int>(from_road.GetHighwayType().value());
    int const to_road_idx = static_cast<int>(to_road.GetHighwayType().value());
    auto const pen = m_turnPenaltyMap.find(from_road_idx * 65535 + to_road_idx);
    if (pen != m_turnPenaltyMap.end())
      penalty = pen->second;
  }

  // Determine if turn crosses traffic based on driving side
  // @TODO We should really account for oneway roads etc.

  bool turn_crosses_traffic;
  if (is_left_hand_traffic)
    turn_crosses_traffic = angle < 0;
  else
    turn_crosses_traffic = angle > 0;

  // Twice as long to turn across traffic than not to
  auto const extra_penalty = turn_crosses_traffic ? 2.0 : 1.0;
  auto const result = fabs(angle) * penalty * extra_penalty;

  return result;
}

double CarEstimator::CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const
{
  double result = road.GetDistance(segment.GetSegmentIdx()) / GetSpeedMpS(purpose, segment, road);

  if (m_trafficStash)
  {
    SpeedGroup const speedGroup = m_trafficStash->GetSpeedGroup(segment);
    ASSERT_LESS(speedGroup, SpeedGroup::Count, ());
    double const trafficFactor = CalcTrafficFactor(speedGroup);
    result *= trafficFactor;
    if (speedGroup != SpeedGroup::Unknown && speedGroup != SpeedGroup::G5)
    {
      // Current time estimation are too optimistic.
      // Need more accurate tuning: traffic lights, traffic jams, road models and so on.
      // Add some penalty to make estimation more realistic.
      /// @todo Make accurate tuning, remove penalty.
      result *= 1.8;
    }
  }

  return result;
}

// EdgeEstimator -----------------------------------------------------------------------------------
// static
shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                                SpeedKMpH const & offroadSpeedKMpH,
                                                shared_ptr<TrafficStash> trafficStash, DataSource * dataSourcePtr,
                                                std::shared_ptr<NumMwmIds> numMwmIds)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return make_shared<PedestrianEstimator>(maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Bicycle: return make_shared<BicycleEstimator>(maxWeighSpeedKMpH, offroadSpeedKMpH);
  case VehicleType::Car:
    return make_shared<CarEstimator>(dataSourcePtr, numMwmIds, trafficStash, maxWeighSpeedKMpH, offroadSpeedKMpH);
  /*
   * VehicleType::Decoder is for use with the TraFF decoder, which creates its EdgeEstimator by
   * explicitly calling the constructor for the appropriate subclass.
   */
  case VehicleType::Decoder: CHECK(false, ("Creating EdgeEstimator for Decoder is not supported")); return nullptr;
  case VehicleType::Count: CHECK(false, ("Can't create EdgeEstimator for", vehicleType)); return nullptr;
  }
  UNREACHABLE();
}

// static
shared_ptr<EdgeEstimator> EdgeEstimator::Create(VehicleType vehicleType, VehicleModelInterface const & vehicleModel,
                                                shared_ptr<TrafficStash> trafficStash, DataSource * dataSourcePtr,
                                                std::shared_ptr<NumMwmIds> numMwmIds)
{
  return Create(vehicleType, vehicleModel.GetMaxWeightSpeed(), vehicleModel.GetOffroadSpeed(), trafficStash,
                dataSourcePtr, numMwmIds);
}
}  // namespace routing
