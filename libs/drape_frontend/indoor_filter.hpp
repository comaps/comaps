#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <string_view>
#include <vector>

namespace df
{
// Returns true if a feature isn't part of the active indoor level and should be skipped at rendering
//
// Indoor-typed features (indoor=*) are always filtered by the active level.
//
// Non-indoor features that carry a level=* tag (POIs, stairs, etc.) are filtered ONLY when:
//   1. Their center is within ~5 m of an indoor polygon rect AND
//   2. Their level intersects the building's available floor set.
// If either is false the feature stays visible. This prevents transit platforms or external
// footways from being hidden just because they happen to share a level tag with a nearby mall.
//
// The building shell (building / building:part) is never filtered: it spans all floors.
//
// When no indoor level is active (kNoActiveLevel) nothing is skipped.
inline bool ShouldSkipIndoorFeature(feature::TypesHolder const & types, std::string_view levelMeta,
                                    double activeLevel, m2::PointD const & featureCenter,
                                    std::vector<m2::RectD> const & indoorPolygonRects,
                                    std::vector<double> const & availableLevels)
{
  if (!indoor::HasActiveLevel(activeLevel))
    return false;

  bool const isIndoor = ftypes::IsIndoorChecker::Instance()(types);
  bool const isLeveled = !levelMeta.empty() && !ftypes::IsBuildingChecker::Instance()(types) &&
                         !ftypes::IsBuildingPartChecker::Instance()(types);
  if (!isIndoor && !isLeveled)
    return false;

  if (!isIndoor)
  {
    // only filter if the feature center is within ~5 m of an indoor polygon.
    double constexpr kProximityDeg = 0.00005;
    if (!indoorPolygonRects.empty())
    {
      bool nearIndoor = false;
      for (auto const & r : indoorPolygonRects)
      {
        m2::RectD expanded(r.minX() - kProximityDeg, r.minY() - kProximityDeg,
                           r.maxX() + kProximityDeg, r.maxY() + kProximityDeg);
        if (expanded.IsPointInside(featureCenter))
        {
          nearIndoor = true;
          break;
        }
      }
      if (!nearIndoor)
        return false;
    }

    // only filter if the feature's level intersects the building floors.
    bool const levelIsInBuilding = std::any_of(availableLevels.begin(), availableLevels.end(),
      [&](double buildingLevel) { return indoor::LevelsContain(levelMeta, buildingLevel); });
    if (!levelIsInBuilding)
      return false;
  }

  return !indoor::LevelsContain(levelMeta, activeLevel);
}
}  // namespace df
