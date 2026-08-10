#include "drape_frontend/indoor_filter.hpp"

#include <algorithm>

namespace df
{
bool ShouldSkipIndoorFeature(feature::TypesHolder const & types, std::string_view levelMeta, double activeLevel,
                             m2::RectD const & featureRect, std::vector<m2::RectD> const & indoorPolygonRects,
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
    // only filter if the feature's bounding rect overlaps an indoor polygon.
    if (!indoorPolygonRects.empty())
    {
      bool const nearIndoor = std::any_of(indoorPolygonRects.begin(), indoorPolygonRects.end(),
        [&featureRect](m2::RectD const & r) { return r.IsIntersect(featureRect); });
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
