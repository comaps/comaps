#pragma once

#include "routing_common/vehicle_model.hpp"

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
