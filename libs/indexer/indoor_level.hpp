#pragma once

#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace indoor
{
// Sentinel meaning "no indoor level is active": the viewport has no indoor data, or the user
// panned away from an indoor context. Features are never level-filtered against this value. Intentionally not NaN.
inline constexpr double kNoActiveLevel = std::numeric_limits<double>::lowest();

// Minimum step between two levels before considering them the same
double constexpr kLevelEpsilon = 0.05;

inline bool LevelsEqual(double lhs, double rhs)
{
  return std::fabs(lhs - rhs) < kLevelEpsilon;
}

// True if level is not kNoActiveLevel.
inline bool HasActiveLevel(double level) { return level != kNoActiveLevel; }

// True for non-building elements with level=* tags: buildings and building parts should stay visible
// underneath indoor mode even if they have level tags. Shared by df::ShouldSkipIndoorFeature and IndoorManager.
bool IsLevelSensitiveType(feature::TypesHolder const & types);

// Parses an OSM level=* value into a list of numeric levels.
// Supported forms: "0", "-1", "1.5", "0;1;2", "0-2" (integer ranges), "-2--1" and "," as a separator fallback.
// Returns an empty vector if the value can't be parsed ("G", "ground").
std::vector<double> ParseLevels(std::string_view s);

// Returns true if the level=* value |s| includes |level|.
// A missing or unparsable value is treated as level 0 (ground floor)
bool LevelsContain(std::string_view s, double level);

// Formats a level for UI display: "0", "-1", "1.5", "0.25".
// Uses locale to return period or comma as appropriate
std::string FormatLevel(double level);

// Expands each rect by X meters in every direction, correcting for differences in latitude
std::vector<m2::RectD> ExpandRectsByMeters(std::vector<m2::RectD> const & rects, double meters);

// Returns a union of touching/overlapping rects for computational efficiency
std::vector<m2::RectD> MergeOverlappingRects(std::vector<m2::RectD> const & rects);
}  // namespace indoor
