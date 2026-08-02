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
// True if feature should be hidden.
//
// Indoor types hidden if inactive.
// Leveled POIs skipped near mismatch.
// Building shell always visible.
inline bool ShouldSkipIndoorFeature(feature::TypesHolder const & types, std::string_view levelMeta,
                                    double activeLevel, m2::PointD const & featureCenter,
                                    std::vector<m2::RectD> const & indoorPolygonRects,
                                    std::vector<double> const & availableLevels)
{
  bool const isIndoor = ftypes::IsIndoorChecker::Instance()(types);

  if (!indoor::HasActiveLevel(activeLevel))
    return isIndoor;

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
