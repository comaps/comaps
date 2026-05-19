#pragma once

#include "routing_common/car_model.hpp"

#include <memory>
#include <unordered_map>

namespace routing
{
struct EmergencyCarModelConfig
{
  double m_speedMultiplier = 1.5;
  bool m_ignoreOneWay = true;
  bool m_allowPassThrough = true;
};

EmergencyCarModelConfig GetDefaultEmergencyCarModelConfig();

class EmergencyCarModel final : public VehicleModelInterface
{
public:
  EmergencyCarModel(std::shared_ptr<VehicleModelInterface> baseModel, EmergencyCarModelConfig config);

  SpeedKMpH GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const override;
  std::optional<HighwayType> GetHighwayType(FeatureTypes const & types) const override;
  double GetMaxWeightSpeed() const override;
  SpeedKMpH const & GetOffroadSpeed() const override;
  bool IsOneWay(FeatureTypes const & types) const override;
  bool IsRoad(FeatureTypes const & types) const override;
  bool IsPassThroughAllowed(FeatureTypes const & types) const override;

private:
  SpeedKMpH ScaleSpeed(SpeedKMpH const & speed) const;

  std::shared_ptr<VehicleModelInterface> m_base;
  EmergencyCarModelConfig m_config;
  SpeedKMpH m_offroadSpeed;
};

class EmergencyCarModelFactory final : public VehicleModelFactoryInterface
{
public:
  explicit EmergencyCarModelFactory(VehicleModelFactory::CountryParentNameGetterFn const & countryParentNameGetterFn,
                                    EmergencyCarModelConfig config = GetDefaultEmergencyCarModelConfig());

  std::shared_ptr<VehicleModelInterface> GetVehicleModel() const override;
  std::shared_ptr<VehicleModelInterface> GetVehicleModelForCountry(std::string const & country) const override;

private:
  std::shared_ptr<VehicleModelInterface> WrapModel(std::shared_ptr<VehicleModelInterface> const & base) const;

  CarModelFactory m_baseFactory;
  EmergencyCarModelConfig m_config;
  mutable std::unordered_map<VehicleModelInterface const *, std::shared_ptr<VehicleModelInterface>> m_wrappedModels;
};
}  // namespace routing
