#pragma once

#include "routing_common/vehicle_model.hpp"

namespace decoder_model
{
  using namespace routing;

  /**
   * @brief One meter per second.
   *
   * The `TraffEstimator` works on distance in meters, not travel time. For code which works with
   * speeds and assumes cost to be time-based, a speed of 1 m/s means such calculations will
   * effectively return distances in meters.
   */
  auto constexpr kOneMpSInKmpH = 3.6;

  /**
   * @brief Penalty factor for non-matching attributes.
   *
   * The weight of a segment is multiplied by this factor for each attribute that does not match.
   * See also `kReducedAttributePenalty` for partial matches.
   */
  auto constexpr kAttributePenalty = 4;

  /**
   * @brief Penalty factor for partially matching attributes.
   *
   * The weight of a segment is multiplied by this factor for each attribute that is a partial match,
   * such as the road class being off by one (e.g. motorway and trunk) or parts of the road ref matching
   * (e.g. `A8` and `S8`, or `B2` and `B2R`).
   * See also `kAttributePenalty` for non-matching attributes.
   */
  auto constexpr kReducedAttributePenalty = 2;

  /*
   * @brief Penalty factor for using a fake segment to get to a nearby road.
   *
   * Offroad penalty applies to direct distance whereas road penalty applies to roads, which can be up
   * to around 3 times the direct distance (theoretically unlimited). Therefore, a factor of 3–4 times
   * the penalty of a well-matched road may be needed to avoid competing with the correct route.
   * On the other hand, a very high offroad penalty would give preference to a poorly matched route
   * over a well-matched one if it is closer to the reference points.
   * Maximum penalty for roads is currently 64 (4 for ramps * 4 for road type * 4 for ref).
   * A well-matched road may still have a penalty of around 4 (twice the reduced attribute penalty, or
   * once the full attribute penalty).
   * A “wrong” road may also just have a penalty of 4 (e.g. road name mismatch, but road class and
   * ramp type match).
   * A value of 16 has worked well for the DE-B2R-SendlingSued-Passauerstrasse test case. (The
   * DE-A10-Werder-GrossKreutz or DE-A115-PotsdamDrewitz-Nuthetal test cases gave incorrect results
   * due to lack of fake segments, which was fixed through truncation and now works correctly even
   * with an offroad penalty of 128.)
   */
  auto constexpr kOffroadPenalty = 16;

  /**
   * @brief Penalty applied to impassable ways.
   */
  auto constexpr kImpassablePenalty = 1.0E4;

  /**
   * @brief The speed assumed for edges that do not coincide with road features, such as pure fake edges.
   *
   * The value is chosen in such a way that fake edges have a comparatively high cost compared to real
   * road features, resulting in a preference for the edges which lie closest to the start or end of the
   * route, unless these require a significant detour.
   */
  SpeedKMpH constexpr kSpeedOffroadKMpH = kOneMpSInKmpH / kOffroadPenalty;
}

namespace routing
{

/**
 * @brief A `VehicleModel` suitable for TraFF location decoding.
 *
 * Each instance can have its own set of feature type-specific access limitations. These specify
 * which roads can be used by this vehicle, and which roads can only be used for destination
 * traffic.
 *
 * `DecoderModel` is similar to `CarModel`, with the following differences:
 *
 * Construction types are routable and treated like regular roads (the decoder may still
 * exclude them on a case-by-case basis).
 *
 * Speed is assumed to be 1 m/s for any routable road, and 1 m/s divided by offline penalty for
 * offroad.
 *
 * Instances are typically retrieved from `DecoderModelFactory` or the `AllLimitsInstance()` method.
 */
class DecoderModel : public VehicleModel
{
public:
  DecoderModel();
  explicit DecoderModel(LimitsInitList const & roadLimits);

  // VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const override;
  SpeedKMpH const & GetOffroadSpeed() const override;

  /**
   * @brief Returns an instance with the default access limitations.
   */
  static DecoderModel const & AllLimitsInstance();

  /**
   * @brief Returns the default access limitations.
   */
  static LimitsInitList const & GetOptions();
};

class DecoderModelFactory : public VehicleModelFactory
{
public:
  DecoderModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
