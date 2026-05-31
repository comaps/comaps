#include "routing_common/decoder_model.hpp"

#include "indexer/classificator.hpp"

namespace decoder_model
{
using namespace routing;

// See model specifics in different countries here:
//   https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access-Restrictions

// See road types here:
//   https://wiki.openstreetmap.org/wiki/Key:highway

/*
 * One meter per second. The TraffEstimator works on distance in meters, not travel time. For code
 * which works with speeds and assumes cost to be time-based, a speed of 1 m/s means such
 * calculations will effectively return distances in meters.
 */
auto constexpr kOneMpSInKmpH = 3.6;

/*
 * Penalty factor for using a fake segment to get to a nearby road.
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

// |kSpeedOffroadKMpH| is a speed which is used for edges that don't lie on road features.
// For example for pure fake edges. In car routing, off road speed for calculation ETA is not used.
// The weight of such edges is considered as 0 seconds. It's especially actual when an airport is
// a start or finish. On the other hand, while route calculation the fake edges are considered
// as quite heavy. The idea behind that is to use the closest edge for the start and the finish
// of the route except for some edge cases.
SpeedKMpH constexpr kSpeedOffroadKMpH = kOneMpSInKmpH / kOffroadPenalty;

HighwayBasedFactors const kDefaultFactors = {
    // {highway class : InOutCityFactor(in city, out city)}

    {HighwayType::HighwayMotorway, InOutCityFactor(1.0)},
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayTrunk, InOutCityFactor(1.0)},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(1.0)},

    {HighwayType::HighwayPrimary, InOutCityFactor(1.0)},
    {HighwayType::HighwayPrimaryLink, InOutCityFactor(1.0)},
    {HighwayType::HighwaySecondary, InOutCityFactor(1.0)},
    {HighwayType::HighwaySecondaryLink, InOutCityFactor(1.0)},

    {HighwayType::HighwayTertiary, InOutCityFactor(1.0)},
    {HighwayType::HighwayTertiaryLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayUnclassified, InOutCityFactor(1.0)},

    {HighwayType::HighwayResidential, InOutCityFactor(1.0)},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(1.0)},

    {HighwayType::HighwayService, InOutCityFactor(1.0)},
    {HighwayType::HighwayRoad, InOutCityFactor(1.0)},
    {HighwayType::HighwayTrack, InOutCityFactor(1.0)},
    {HighwayType::ManMadePier, InOutCityFactor(1.0)},

    {HighwayType::RouteFerry, InOutCityFactor(1.0)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(1.0)},

    // Generic construction type
    {HighwayType::HighwayConstruction, InOutCityFactor(1.0)},

    // Construction types for each highway type:
    {HighwayType::HighwayConstructionMotorway, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionMotorwayLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionTrunk, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionTrunkLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionPrimary, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionPrimaryLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionSecondary, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionSecondaryLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionTertiary, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionTertiaryLink, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionResidential, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionUnclassified, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionService, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionLivingStreet, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionRoad, InOutCityFactor(1.0)},
    {HighwayType::HighwayConstructionTrack, InOutCityFactor(1.0)},
};

HighwayBasedSpeeds const kDefaultSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city(weight, eta), out city(weight eta))}
    {HighwayType::HighwayMotorway, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayService, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    // The router truncates types to two levels, so we need this in addition to the long construction types
    {HighwayType::HighwayConstruction, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    // Construction conterparts to each of the types above
    // (needed for the map generator to include them in the routing section)
    {HighwayType::HighwayConstructionMotorway, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionMotorwayLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionTrunk, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionTrunkLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionPrimary, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionPrimaryLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionSecondary, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionSecondaryLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionTertiary, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionTertiaryLink, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionResidential, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionUnclassified, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionService, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionLivingStreet, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionRoad, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::HighwayConstructionTrack, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    // Non-highway types
    {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::RouteFerry, InOutCitySpeedKMpH(kOneMpSInKmpH)},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH(kOneMpSInKmpH)}};

/*
 * The number of entries must match `kHighwayBasedFactors` and `kHighwayBasedSpeeds`
 * in `decoder_model_coefs.hpp`.
 */
VehicleModel::LimitsInitList const kDefaultOptions = {
    // {HighwayType, passThroughAllowed}
    {HighwayType::HighwayMotorway, true},    {HighwayType::HighwayMotorwayLink, true},
    {HighwayType::HighwayTrunk, true},       {HighwayType::HighwayTrunkLink, true},
    {HighwayType::HighwayPrimary, true},     {HighwayType::HighwayPrimaryLink, true},
    {HighwayType::HighwaySecondary, true},   {HighwayType::HighwaySecondaryLink, true},
    {HighwayType::HighwayTertiary, true},    {HighwayType::HighwayTertiaryLink, true},
    {HighwayType::HighwayResidential, true}, {HighwayType::HighwayUnclassified, true},
    {HighwayType::HighwayService, true},     {HighwayType::HighwayLivingStreet, true},
    {HighwayType::HighwayRoad, true},        {HighwayType::HighwayTrack, true},
    // The router truncates types to two levels, so we need this in addition to the long construction types
    {HighwayType::HighwayConstruction, true},
    // Construction conterparts to each of the types above
    // (needed for the map generator to include them in the routing section)
    {HighwayType::HighwayConstructionMotorway, true},    {HighwayType::HighwayConstructionMotorwayLink, true},
    {HighwayType::HighwayConstructionTrunk, true},       {HighwayType::HighwayConstructionTrunkLink, true},
    {HighwayType::HighwayConstructionPrimary, true},     {HighwayType::HighwayConstructionPrimaryLink, true},
    {HighwayType::HighwayConstructionSecondary, true},   {HighwayType::HighwayConstructionSecondaryLink, true},
    {HighwayType::HighwayConstructionTertiary, true},    {HighwayType::HighwayConstructionTertiaryLink, true},
    {HighwayType::HighwayConstructionResidential, true}, {HighwayType::HighwayConstructionUnclassified, true},
    {HighwayType::HighwayConstructionService, true},     {HighwayType::HighwayConstructionLivingStreet, true},
    {HighwayType::HighwayConstructionRoad, true},        {HighwayType::HighwayConstructionTrack, true},
    // Non-highway types
    {HighwayType::RouteShuttleTrain, true},  {HighwayType::RouteFerry, true},
    {HighwayType::ManMadePier, true}};

VehicleModel::LimitsInitList NoPassThroughLivingStreet()
{
  auto res = kDefaultOptions;
  for (auto & e : res)
    if (e.m_type == HighwayType::HighwayLivingStreet)
      e.m_isPassThroughAllowed = false;
  return res;
}

VehicleModel::LimitsInitList NoPassThroughService(VehicleModel::LimitsInitList res = kDefaultOptions)
{
  for (auto & e : res)
    if (e.m_type == HighwayType::HighwayService)
      e.m_isPassThroughAllowed = false;
  return res;
}

VehicleModel::LimitsInitList NoTrack()
{
  VehicleModel::LimitsInitList res;
  res.reserve(kDefaultOptions.size() - 1);
  for (auto const & e : kDefaultOptions)
    if (e.m_type != HighwayType::HighwayTrack)
      res.push_back(e);
  return res;
}

VehicleModel::LimitsInitList NoPassThroughTrack()
{
  auto res = kDefaultOptions;
  for (auto & e : res)
    if (e.m_type == HighwayType::HighwayTrack)
      e.m_isPassThroughAllowed = false;
  return res;
}

/// @todo Should make some compare constrains (like in CarModel_TrackVsGravelTertiary test)
/// to better fit these factors with reality. I have no idea, how they were set.
VehicleModel::SurfaceInitList const kDecoderSurface = {
    // {{surfaceType, surfaceType}, {weightFactor, etaFactor}}
    {{"psurface", "paved_good"}, {1.0, 1.0}},
    {{"psurface", "paved_bad"}, {0.6, 0.7}},
    {{"psurface", "unpaved_good"}, {0.4, 0.7}},
    {{"psurface", "unpaved_bad"}, {0.2, 0.3}}};
}  // namespace decoder_model

namespace routing
{
DecoderModel::DecoderModel() : DecoderModel(decoder_model::kDefaultOptions) {}

DecoderModel::DecoderModel(VehicleModel::LimitsInitList const & roadLimits)
  : VehicleModel(classif(), roadLimits, decoder_model::kDecoderSurface,
                 {decoder_model::kDefaultSpeeds, decoder_model::kDefaultFactors})
{
  ASSERT_EQUAL(decoder_model::kDefaultFactors.size(), decoder_model::kDefaultSpeeds.size(), ());
  ASSERT_EQUAL(decoder_model::kDefaultOptions.size(), decoder_model::kDefaultSpeeds.size(), ());

  std::vector<std::string> hwtagYesCar = {"hwtag", "yescar"};
  auto const & cl = classif();

  m_noType = cl.GetTypeByPath({"hwtag", "nocar"});
  m_yesType = cl.GetTypeByPath(hwtagYesCar);

  // Set small track speed if highway is not in kDefaultSpeeds (path, pedestrian), but marked as yescar.
  AddAdditionalRoadTypes(cl, {{std::move(hwtagYesCar), decoder_model::kDefaultSpeeds.at(HighwayType::HighwayTrack)}});

  // Set max possible (reasonable) car speed. See EdgeEstimator::CalcHeuristic.
  SpeedKMpH constexpr kMaxCarSpeedKMpH(200.0);
  CHECK_LESS(m_maxModelSpeed, kMaxCarSpeedKMpH, ());
  m_maxModelSpeed = kMaxCarSpeedKMpH;
}

SpeedKMpH DecoderModel::GetSpeed(FeatureTypes const & types, SpeedParams const & speedParams) const
{
  return GetTypeSpeedImpl(types, speedParams, true /* isCar */);
}

SpeedKMpH const & DecoderModel::GetOffroadSpeed() const
{
  return decoder_model::kSpeedOffroadKMpH;
}

// static
DecoderModel const & DecoderModel::AllLimitsInstance()
{
  static DecoderModel const instance;
  return instance;
}

// static
VehicleModel::LimitsInitList const & DecoderModel::GetOptions()
{
  return decoder_model::kDefaultOptions;
}

DecoderModelFactory::DecoderModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn)
  : VehicleModelFactory(countryParentNameGetterFn)
{
  using namespace decoder_model;
  using std::make_shared;

  // Names must be the same with country names from countries.txt
  m_models[""] = make_shared<DecoderModel>();

  /*
   * This section should mirror the definitions in `CarModel`. Any deviations should be considered
   * a bug, unless there is a comment stating that this is intentional (and ideally also the reason
   * it is needed).
   */
  m_models["Austria"] = make_shared<DecoderModel>(NoPassThroughLivingStreet());
  m_models["Belarus"] = make_shared<DecoderModel>(NoPassThroughLivingStreet());
  m_models["Brazil"] = make_shared<DecoderModel>(NoPassThroughService(NoPassThroughTrack()));
  m_models["Denmark"] = make_shared<DecoderModel>(NoTrack());
  m_models["Germany"] = make_shared<DecoderModel>(NoPassThroughTrack());
  m_models["Hungary"] = make_shared<DecoderModel>(NoPassThroughLivingStreet());
  m_models["Poland"] = make_shared<DecoderModel>(NoPassThroughService());
  m_models["Romania"] = make_shared<DecoderModel>(NoPassThroughLivingStreet());
  m_models["Russian Federation"] = make_shared<DecoderModel>(NoPassThroughService(NoPassThroughLivingStreet()));
  m_models["Slovakia"] = make_shared<DecoderModel>(NoPassThroughLivingStreet());
  m_models["Ukraine"] = make_shared<DecoderModel>(NoPassThroughService(NoPassThroughLivingStreet()));
}
}  // namespace routing
