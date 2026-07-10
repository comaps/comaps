#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"

#include <string_view>

namespace df
{
// Returns true if an indoor-typed feature must be skipped at rendering
// because its level=* metadata doesn't include the active indoor level.
// Non-indoor features are never skipped.
inline bool ShouldSkipIndoorFeature(feature::TypesHolder const & types, std::string_view levelMeta, double activeLevel)
{
  if (!ftypes::IsIndoorChecker::Instance()(types))
    return false;
  return !indoor::LevelsContain(levelMeta, activeLevel);
}
}  // namespace df
