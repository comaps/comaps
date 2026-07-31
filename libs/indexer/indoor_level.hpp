#pragma once

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace indoor
{
// Sentinel meaning "no indoor level is active": the viewport has no indoor data, or the user
// panned away from an indoor context. Features are never level-filtered against this value.
inline constexpr double kNoActiveLevel = std::numeric_limits<double>::quiet_NaN();

// True if |level| is a real, active indoor level rather than the kNoActiveLevel sentinel.
inline bool HasActiveLevel(double level) { return !std::isnan(level); }

// Parses an OSM level=* value into a list of numeric levels.
// Supported forms: "0", "-1", "1.5", "0;1;2", "0-2" (integer ranges), "-2--1" and "," as a separator fallback.
// Returns an empty vector if the value can't be parsed (e.g. "G", "ground").
std::vector<double> ParseLevels(std::string_view s);

// Returns true if the level=* value |s| includes |level|.
// A missing or unparsable value is treated as level 0 (ground floor)
bool LevelsContain(std::string_view s, double level);

// Formats a level for UI display: "0", "-1", "1.5".
std::string FormatLevel(double level);
}  // namespace indoor
