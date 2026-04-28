#pragma once

#include "routing_common/vehicle_model.hpp"

// These are default car model coefficients for open source developers.
// Both maps, as well as `car_model::kDefaultOptions` (in `car_model.cpp`), must have the same number of entries.

namespace routing
{
/**
 * @brief Speed to indicate that a segment is impassable.
 */
/*
 * 1.0E-4 is the inverse of 1.0E4, which is used in `edge_estimator.cpp` as the penalty factor
 * for the weight of a segment with `SpeedGroup::TempBlock`, i.e. currently impassable.
 * Using its inverse as the speed is equivalent to 1 km/h with a penalty factor of 1.0E4.
 * The lowest speed is currently 5 km/h (for `HighwayTrack`), thus 1 m of an impassable way has the
 * same weight as at least 5 km of any routable way.
 * This has worked well in tests. In theory, we could increase it further to 1.0E-8, resulting in
 * 0.8 m of an impassable way costing as much as at least 40,000 km (earth circumference) of any
 * routable way.
 */
auto const kImpassableSpeedKMpH = 1.0E-4;

HighwayBasedFactors const kHighwayBasedFactors = {
    // {highway class : InOutCityFactor(in city, out city)}

    // Tier 1 (decrease ETA a bit according to the tests):
    {HighwayType::HighwayMotorway, InOutCityFactor(SpeedFactor{0.95 /* weight */, 0.93 /* eta */})},
    // See XXX_KeepMotorway integration tests.
    {HighwayType::HighwayMotorwayLink, InOutCityFactor(0.75)},  // 0.20 less
    {HighwayType::HighwayTrunk, InOutCityFactor(0.90)},
    {HighwayType::HighwayTrunkLink, InOutCityFactor(0.80)},  // 0.15 less

    // Tier 2:
    {HighwayType::HighwayPrimary,
     InOutCityFactor(SpeedFactor{0.95 /* weight */, 0.90 /* eta */} /* in city */, 0.85 /* out city */)},
    {HighwayType::HighwayPrimaryLink, InOutCityFactor(0.70 /* in city */, 0.75 /* out city */)},  // 0.10 less
    {HighwayType::HighwaySecondary, InOutCityFactor(0.80 /* in city */, 0.85 /* out city */)},
    {HighwayType::HighwaySecondaryLink, InOutCityFactor(0.70 /* in city */, 0.75 /* out city */)},  // 0.10 less

    // Tier 3:
    {HighwayType::HighwayTertiary,
     InOutCityFactor(0.70 /* in city */, 0.75 /* out city */)},  // 0.10 less than Secondary
    {HighwayType::HighwayTertiaryLink, InOutCityFactor(0.60 /* in city */, 0.65 /* out city */)},  // 0.10 less
    {HighwayType::HighwayUnclassified, InOutCityFactor(0.70 /* in city */, 0.75 /* out city */)},

    // Tier 4:
    {HighwayType::HighwayResidential, InOutCityFactor(0.70)},
    {HighwayType::HighwayLivingStreet, InOutCityFactor(0.70)},

    // The rest:
    // By VNG: Changed 0.3 -> 0.95 for Road and 0.3 -> 1.0 for Track.
    // They are already have very small speeds (10, 5 respectively).
    // There are no (99%) traffic lights or pedestrian crossings on this kind of roads.
    {HighwayType::HighwayService, InOutCityFactor(0.70)},
    {HighwayType::HighwayRoad, InOutCityFactor(0.90)},
    {HighwayType::HighwayTrack, InOutCityFactor(1.0)},
    {HighwayType::ManMadePier, InOutCityFactor(0.90)},

    {HighwayType::RouteFerry, InOutCityFactor(0.90)},
    {HighwayType::RouteShuttleTrain, InOutCityFactor(0.90)},

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

HighwayBasedSpeeds const kHighwayBasedSpeeds = {
    // {highway class : InOutCitySpeedKMpH(in city, out city)}

    // Tier 1:
    {HighwayType::HighwayMotorway, InOutCitySpeedKMpH(118.0 /* in city */, 124.0 /* out city */)},
    {HighwayType::HighwayMotorwayLink, InOutCitySpeedKMpH(109.00 /* in city */, 115.00 /* out city */)},
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(90.00 /* in city */, 103.00 /* out city */)},
    {HighwayType::HighwayTrunkLink, InOutCitySpeedKMpH(77.00 /* in city */, 91.00 /* out city */)},

    // Tier 2:
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(65.00 /* in city */, 82.00 /* out city */)},
    {HighwayType::HighwayPrimaryLink, InOutCitySpeedKMpH(58.00 /* in city */, 72.00 /* out city */)},
    {HighwayType::HighwaySecondary, InOutCitySpeedKMpH(60.00 /* in city */, 70.00 /* out city */)},
    {HighwayType::HighwaySecondaryLink, InOutCitySpeedKMpH(48.00 /* in city */, 56.00 /* out city */)},

    // Tier 3:
    {HighwayType::HighwayTertiary, InOutCitySpeedKMpH(50.00 /* in city */, 50.00 /* out city */)},
    {HighwayType::HighwayTertiaryLink, InOutCitySpeedKMpH({40.95, 34.97} /* in city */, {45.45, 39.73} /* out city */)},
    {HighwayType::HighwayUnclassified, InOutCitySpeedKMpH(30.00 /* in city */, 40.00 /* out city */)},

    // Tier 4:
    {HighwayType::HighwayResidential, InOutCitySpeedKMpH({20.00, 26.00} /* in city */, {26.00, 26.00} /* out city */)},
    {HighwayType::HighwayLivingStreet, InOutCitySpeedKMpH(10.00 /* in city */, 10.00 /* out city */)},

    // The rest:
    {HighwayType::HighwayService, InOutCitySpeedKMpH(15.00 /* in city */, 15.00 /* out city */)},
    {HighwayType::HighwayRoad, InOutCitySpeedKMpH(10.00 /* in city */, 10.00 /* out city */)},
    {HighwayType::HighwayTrack, InOutCitySpeedKMpH(5.00 /* in city */, 5.00 /* out city */)},
    {HighwayType::ManMadePier, InOutCitySpeedKMpH({17.00, 10.00} /* in city */, {17.00, 10.00} /* out city */)},

    {HighwayType::RouteFerry, InOutCitySpeedKMpH(10.00 /* in city */, 10.00 /* out city */)},
    {HighwayType::RouteShuttleTrain, InOutCitySpeedKMpH(25.00 /* in city */, 25.00 /* out city */)},

    // Generic and long construction types:
    // They are needed for traffic location decoding but we don’t want to route drivers through
    // them, which is accomplished by setting their speed to kImpassableSpeedKMpH.
    {HighwayType::HighwayConstruction, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionMotorway, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionMotorwayLink, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionTrunk, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionTrunkLink, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionPrimary, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionPrimaryLink, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionSecondary, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionSecondaryLink, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionTertiary, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionTertiaryLink, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionResidential, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionUnclassified, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionService, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionLivingStreet, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionRoad, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
    {HighwayType::HighwayConstructionTrack, InOutCitySpeedKMpH(kImpassableSpeedKMpH /* in city */, kImpassableSpeedKMpH /* out city */)},
};
}  // namespace routing
