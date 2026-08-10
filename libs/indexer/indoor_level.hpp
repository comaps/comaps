#pragma once

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
// panned away from an indoor context. Features are never level-filtered against this value.
//
// Deliberately not NaN: this project builds with -ffast-math (CMakeLists.txt), under which
// std::isnan() and NaN comparisons are unreliable (GCC/Clang may assume no NaNs exist and fold
// isnan() to a constant false). std::numeric_limits<double>::lowest() is a finite value that no
// real OSM level=* tag will ever equal, so a plain != comparison stays correct under -ffast-math.
inline constexpr double kNoActiveLevel = std::numeric_limits<double>::lowest();

// Minimum step between two levels before considering them the same
double constexpr kLevelEpsilon = 0.05;

inline bool LevelsEqual(double lhs, double rhs)
{
  return std::fabs(lhs - rhs) < kLevelEpsilon;
}

// True if |level| is a real, active indoor level rather than the kNoActiveLevel sentinel.
inline bool HasActiveLevel(double level) { return level != kNoActiveLevel; }

// Parses an OSM level=* value into a list of numeric levels.
// Supported forms: "0", "-1", "1.5", "0;1;2", "0-2" (integer ranges), "-2--1" and "," as a separator fallback.
// Returns an empty vector if the value can't be parsed (e.g. "G", "ground").
std::vector<double> ParseLevels(std::string_view s);

// Returns true if the level=* value |s| includes |level|.
// A missing or unparsable value is treated as level 0 (ground floor)
bool LevelsContain(std::string_view s, double level);

// Formats a level for UI display: "0", "-1", "1.5", "0.25".
// Uses locale to return period or comma as appropriate
std::string FormatLevel(double level);

// Expands each rect by |meters| in every direction, correcting for longitude compression at
// high latitudes (a flat mercator-degree offset would over-expand near the equator and
// under-expand near the poles).
std::vector<m2::RectD> ExpandRectsByMeters(std::vector<m2::RectD> const & rects, double meters);

// Merges touching/overlapping rects into their bounding union, e.g. so a building's many
// individually-mapped rooms collapse into a handful of rects instead of one per room.
std::vector<m2::RectD> MergeOverlappingRects(std::vector<m2::RectD> const & rects);
}  // namespace indoor
