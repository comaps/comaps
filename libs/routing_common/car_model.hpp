#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

/**
 * @brief A `VehicleModel` suitable for car routing.
 *
 * Each instance can have its own set of feature type-specific access limitations. These specify
 * which roads can be used by this vehicle, and which roads can only be used for destination
 * traffic.
 *
 * Instances are typically retrieved from `CarModelFactory` or the `AllLimitsInstance()` method.
 */
class CarModel : public VehicleModel
{
public:
  CarModel();
  explicit CarModel(LimitsInitList const & roadLimits);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const override;
  SpeedKMpH const & GetOffroadSpeed() const override;

  /**
   * @brief Returns an instance with the default access limitations.
   */
  static CarModel const & AllLimitsInstance();

  /**
   * @brief Returns the default access limitations.
   */
  static LimitsInitList const & GetOptions();

  static SurfaceInitList const & GetSurfaces();
};

class CarModelFactory : public VehicleModelFactory
{
public:
  CarModelFactory(CountryParentNameGetterFn const & countryParentNameGetterF);
};
}  // namespace routing
