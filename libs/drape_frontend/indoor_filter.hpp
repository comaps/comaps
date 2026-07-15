#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"

#include <string_view>

namespace df
{
// Returns true if a floor-bound feature must be skipped at rendering because its level=* metadata
// doesn't include the active indoor level. Floor-bound means an indoor-typed feature, or any
// feature carrying a level=* tag (POIs, but also stairs, elevators, footways, etc.) so it isn't
// drawn over every other floor.
//
// The building shell itself (building / building:part) is excluded: it spans all floors and is
// governed by the 3D-buildings path, not the indoor level selector.
//
// When no indoor level is active (kNoActiveLevel, e.g. the viewport has no indoor data or the user
// panned away) nothing is skipped, so level-tagged features stay visible in the ordinary map.
inline bool ShouldSkipIndoorFeature(feature::TypesHolder const & types, std::string_view levelMeta, double activeLevel)
{
  if (!indoor::HasActiveLevel(activeLevel))
    return false;

  bool const isIndoor = ftypes::IsIndoorChecker::Instance()(types);
  bool const isLeveled = !levelMeta.empty() && !ftypes::IsBuildingChecker::Instance()(types) &&
                         !ftypes::IsBuildingPartChecker::Instance()(types);
  if (!isIndoor && !isLeveled)
    return false;

  return !indoor::LevelsContain(levelMeta, activeLevel);
}
}  // namespace df
