#include "routing_common/emergency_car_model.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace routing
{
EmergencyCarModelConfig GetDefaultEmergencyCarModelConfig() { return {}; }

EmergencyCarModel::EmergencyCarModel(std::shared_ptr<VehicleModelInterface> baseModel, EmergencyCarModelConfig config)
  : m_base(std::move(baseModel))
  , m_config(config)
  , m_offroadSpeed()
{
  CHECK(m_base != nullptr, ());
  if (m_config.m_speedMultiplier <= 0.0)
    m_config.m_speedMultiplier = 1.0;

  m_offroadSpeed = ScaleSpeed(m_base->GetOffroadSpeed());
}

SpeedKMpH EmergencyCarModel::GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const
{
  auto const baseSpeed = m_base->GetSpeed(types, speedParams);
  return ScaleSpeed(baseSpeed);
}

std::optional<HighwayType> EmergencyCarModel::GetHighwayType(FeatureTypes const & types) const
{
  return m_base->GetHighwayType(types);
}

double EmergencyCarModel::GetMaxWeightSpeed() const
{
  return m_base->GetMaxWeightSpeed() * m_config.m_speedMultiplier;
}

SpeedKMpH const & EmergencyCarModel::GetOffroadSpeed() const
{
  return m_offroadSpeed;
}

bool EmergencyCarModel::IsOneWay(FeatureTypes const & types) const
{
  if (m_config.m_ignoreOneWay)
    return false;
  return m_base->IsOneWay(types);
}

bool EmergencyCarModel::IsRoad(FeatureTypes const & types) const
{
  return m_base->IsRoad(types);
}

bool EmergencyCarModel::IsPassThroughAllowed(FeatureTypes const & types) const
{
  if (m_config.m_allowPassThrough)
    return m_base->IsRoad(types);
  return m_base->IsPassThroughAllowed(types);
}

SpeedKMpH EmergencyCarModel::ScaleSpeed(SpeedKMpH const & speed) const
{
  auto const weight = speed.m_weight * m_config.m_speedMultiplier;
  auto eta = speed.m_eta;
  if (eta != kNotUsed)
    eta *= m_config.m_speedMultiplier;
  return {weight, eta};
}

EmergencyCarModelFactory::EmergencyCarModelFactory(
    VehicleModelFactory::CountryParentNameGetterFn const & countryParentNameGetterFn, EmergencyCarModelConfig config)
  : m_baseFactory(countryParentNameGetterFn)
  , m_config(config)
{}

std::shared_ptr<VehicleModelInterface> EmergencyCarModelFactory::GetVehicleModel() const
{
  return WrapModel(m_baseFactory.GetVehicleModel());
}

std::shared_ptr<VehicleModelInterface> EmergencyCarModelFactory::GetVehicleModelForCountry(
    std::string const & country) const
{
  return WrapModel(m_baseFactory.GetVehicleModelForCountry(country));
}

std::shared_ptr<VehicleModelInterface> EmergencyCarModelFactory::WrapModel(
    std::shared_ptr<VehicleModelInterface> const & base) const
{
  auto const key = base.get();
  auto it = m_wrappedModels.find(key);
  if (it != m_wrappedModels.end())
    return it->second;

  auto wrapped = std::make_shared<EmergencyCarModel>(base, m_config);
  m_wrappedModels.emplace(key, wrapped);
  return wrapped;
}
}  // namespace routing
