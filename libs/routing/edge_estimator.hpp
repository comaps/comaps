#pragma once

#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point_with_altitude.hpp"

#include <memory>

#include "3party/ankerl/unordered_dense.h"

class DataSource;

namespace routing
{
class RoadGeometry;
class Segment;
class TrafficStash;

class EdgeEstimator
{
public:
  /**
   * @brief The purpose for which cost calculations are to be used.
   *
   * A number of cost estimation functions take `Purpose` as an argument and may return different
   * values depending on the value of that argument.
   */
  enum class Purpose
  {
    /**
     * @brief Indicates that cost calculations are for the purpose of choosing the best route.
     */
    Weight,
    /**
     * @brief Indicates that cost calculations are for the purpose of calculating the estimated time
     * of arrival.
     */
    ETA
  };

  /**
   * @brief Constructs a new `EdgeEstimator`.
   *
   * @param vehicleType The vehicle type.
   * @param maxWeightSpeedKMpH The maximum speed for the vehicle on a road.
   * @param offroadSpeedKMpH The maximum speed for the vehicle on an off-road link.
   * @param dataSourcePtr
   * @param numMwmIds
   */
  EdgeEstimator(VehicleType vehicleType, double maxWeightSpeedKMpH, SpeedKMpH const & offroadSpeedKMpH,
                DataSource * dataSourcePtr = nullptr, std::shared_ptr<NumMwmIds> numMwmIds = nullptr);
  virtual ~EdgeEstimator() = default;

  /**
   * @brief Calculates the heuristic for two points.
   *
   * The heuristic is used by the A* routing algorithm when choosing the next point to examine. It
   * must be less than, or equal to, the lowest possible cost of traveling from one point to the
   * other. Zero is an admissible heuristic, but effectively downgrades the A* algorithm to behave
   * exactly like the Dijkstra algorithm, of which A* is an improved version. A good heuristic is as
   * close as possible to the actual cost, without violating the aforementioned requirement.
   *
   * @param from The start point for the part of the route for which the heuristic is to be calculated.
   * @param to The destination point for the part of the route for which the heuristic is to be calculated.
   * @return The heuristic, expressed as travel time in seconds.
   */
  double CalcHeuristic(ms::LatLon const & from, ms::LatLon const & to) const;

  /**
   * @brief Estimates travel time between two points along a leap (fake) edge using real features.
   *
   * Estimates time in seconds it takes to go from point `from` to point `to` along a leap (fake)
   * edge `from`-`to` using real features.
   *
   * Note 1. The result of the method should be used if it is necessary to add a leap (fake) edge
   * (`from`, `to`) in road graph.
   *
   * Note 2. The result of the method should be less or equal to `CalcHeuristic(from, to)`.
   *
   * Note 3. It is assumed here that `CalcLeapWeight(p1, p2) == CalcLeapWeight(p2, p1)`.
   *
   * @todo Note 2 looks like a typo, presumably the result of this method should be no less than the
   * heuristic (otherwise the heuristic might not satisfy the requirements of A*).
   *
   * @param from The start point.
   * @param to The destination point.
   * @param mwmId
   * @return Travel time in seconds.
   */
  double CalcLeapWeight(ms::LatLon const & from, ms::LatLon const & to, NumMwmId mwmId = kFakeNumMwmId);

  /**
   * @brief Returns the maximum speed this `EdgeEstimator` instance assumes for any road.
   * @return The speed in m/s.
   */
  double GetMaxWeightSpeedMpS() const;

  /**
   * @brief Estimates travel time between two points along a direct fake edge.
   *
   * Estimates time in seconds it takes to go from point `from` to point `to` along direct fake edge.
   *
   * @param from The start point.
   * @param to The destination point.
   * @param purpose The purpose for which the result is to be used.
   * @return Travel time in seconds.
   */
  virtual double CalcOffroad(ms::LatLon const & from, ms::LatLon const & to, Purpose purpose) const;

  /**
   * @brief Returns the travel time along a segment.
   *
   * @param segment The segment.
   * @param road The road geometry (speed, restrictions, points) for the road which the segment is a part of.
   * @param purpose The purpose for which the result is to be used.
   * @return Travel time in seconds.
   */
  virtual double CalcSegmentWeight(Segment const & segment, RoadGeometry const & road, Purpose purpose) const = 0;

  /**
   * @brief Returns the penalty for making a U turn.
   *
   * The penalty is a fixed amount of time, determined by the implementation.
   *
   * @param purpose The purpose for which the result is to be used.
   * @return The penalty in seconds.
   */
  virtual double GetUTurnPenalty(Purpose purpose) const = 0;
  
  virtual double GetTurnPenalty(Purpose purpose, double angle, RoadGeometry const & from_road,
                                RoadGeometry const & to_road, bool is_left_hand_traffic = false) const = 0;

  /**
   * @brief Returns the penalty for using a ferry or rail transit link.
   *
   * The penalty is a fixed amount of time, determined by the implementation. It applies once per
   * link, hence it needs to cover the sum of the time for boarding and unboarding.
   *
   * @param purpose The purpose for which the result is to be used.
   * @return The penalty in seconds.
   */
  virtual double GetFerryLandingPenalty(Purpose purpose) const = 0;

  /**
   * @brief Whether access restrictions are ignored.
   *
   * A return value of false indicates that access restrictions should be observed, which is the
   * default behavior for a routing use case. If true, it indicates that routing should ignore
   * access restrictions. This is needed to resolve traffic message locations; it could also be
   * used e.g. for emergency vehicle use cases.
   *
   * This implementation always returns false.
   */
  virtual bool IsAccessIgnored() const { return false; }

  /**
   * @brief Creates an `EdgeEstimator` based on maximum speeds.
   *
   * @param vehicleType The vehicle type.
   * @param maxWeighSpeedKMpH The maximum speed for the vehicle on a road.
   * @param offroadSpeedKMpH The maximum speed for the vehicle on an off-road link.
   * @param trafficStash The traffic stash (used only for some vehicle types).
   * @param dataSourcePtr
   * @param numMwmIds
   * @return The `EdgeEstimator` instance.
   */
  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType, double maxWeighSpeedKMpH,
                                               SpeedKMpH const & offroadSpeedKMpH,
                                               std::shared_ptr<TrafficStash> trafficStash, DataSource * dataSourcePtr,
                                               std::shared_ptr<NumMwmIds> numMwmIds);

  /**
   * @brief Creates an `EdgeEstimator` based on a vehicle model.
   *
   * This is a convenience wrapper around `Create(VehicleType, double, SpeedKMpH const &,
   * std::shared_ptr<TrafficStash>, DataSource *, std::shared_ptr<NumMwmIds>)`, which takes a
   * `VehicleModel` and derives the maximum speeds for the vehicle from that.
   *
   * @param vehicleType The vehicle type.
   * @param vehicleModel
   * @param trafficStash The traffic stash (used only for some vehicle types).
   * @param dataSourcePtr
   * @param numMwmIds
   * @return The `EdgeEstimator` instance.
   */
  static std::shared_ptr<EdgeEstimator> Create(VehicleType vehicleType, VehicleModelInterface const & vehicleModel,
                                               std::shared_ptr<TrafficStash> trafficStash, DataSource * dataSourcePtr,
                                               std::shared_ptr<NumMwmIds> numMwmIds);

protected:
  VehicleType const m_vehicleType;
  double m_defaultPenalty = 0.0;
  ankerl::unordered_dense::map<int, double> m_turnPenaltyMap;

private:
  double const m_maxWeightSpeedMpS;
  SpeedKMpH const m_offroadSpeedKMpH;

  // DataSource * m_dataSourcePtr;
  // std::shared_ptr<NumMwmIds> m_numMwmIds;
  // ankerl::unordered_dense::map<NumMwmId, double> m_leapWeightSpeedMpS;

  /**
   * @brief Computes the default speed for leap (fake) segments.
   *
   * The result is used by `GetLeapWeightSpeed()`.
   *
   * @return Speed in m/s.
   */
  double ComputeDefaultLeapWeightSpeed() const;

  /**
   * @brief Returns the deafult speed for leap (fake) segments for a given MWM.
   * @param mwmId
   * @return Speed in m/s.
   */
  double GetLeapWeightSpeed(NumMwmId mwmId);
  // double LoadLeapWeightSpeed(NumMwmId mwmId);
};

/**
 * @brief Calculates the climb penalty for pedestrians.
 *
 * The climb penalty is a factor which can be multiplied with the cost of an edge which goes uphill
 * or downhill. The factor for no penalty is 1, i.e. the cost of the edge is not changed.
 *
 * The climb penalty may depend on the mode of transportation, the ascent or descent, as well as the
 * altitude (allowing for different penalties at greater altitudes).
 *
 * @param purpose The purpose for which the result is to be used.
 * @param tangent The tangent of the ascent or descent (10% would be 0.1 for ascent, -0.1 for descent).
 * @param altitudeM The altitude in meters.
 * @return The climb penalty, as a factor.
 */
double GetPedestrianClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM);

/**
 * @brief Calculates the climb penalty for cyclists.
 *
 * The climb penalty is a factor which can be multiplied with the cost of an edge which goes uphill
 * or downhill. The factor for no penalty is 1, i.e. the cost of the edge is not changed.
 *
 * The climb penalty may depend on the mode of transportation, the ascent or descent, as well as the
 * altitude (allowing for different penalties at greater altitudes).
 *
 * @param purpose The purpose for which the result is to be used.
 * @param tangent The tangent of the ascent or descent (10% would be 0.1 for ascent, -0.1 for descent).
 * @param altitudeM The altitude in meters.
 * @return The climb penalty, as a factor.
 */
double GetBicycleClimbPenalty(EdgeEstimator::Purpose purpose, double tangent, geometry::Altitude altitudeM);

}  // namespace routing
