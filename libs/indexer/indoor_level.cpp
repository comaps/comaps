#include "indexer/indoor_level.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace indoor
{
namespace
{
double constexpr kLevelEpsilon = 1e-9;

// Splits a token like "0-2" or "-2--1" into a from-to range.
// An interior '-' is a range separator; a leading '-' is a sign.
bool ParseRange(std::string_view token, double & from, double & to)
{
  size_t sepPos = std::string_view::npos;
  for (size_t i = 1; i < token.size(); ++i)
  {
    if (token[i] == '-')
    {
      sepPos = i;
      break;
    }
  }
  if (sepPos == std::string_view::npos)
    return false;

  return strings::to_double(token.substr(0, sepPos), from) && strings::to_double(token.substr(sepPos + 1), to) &&
         from <= to;
}

void AppendUnique(std::vector<double> & levels, double level)
{
  for (double const existing : levels)
    if (std::fabs(existing - level) < kLevelEpsilon)
      return;
  levels.push_back(level);
}
}  // namespace

std::vector<double> ParseLevels(std::string_view s)
{
  std::vector<double> levels;
  bool failed = false;

  strings::Tokenize(s, ";,", [&](std::string_view token)
  {
    if (failed)
      return;

    strings::Trim(token);
    if (token.empty())
      return;

    double level;
    if (strings::to_double(token, level))
    {
      AppendUnique(levels, level);
      return;
    }

    double from, to;
    if (ParseRange(token, from, to))
    {
      // Expand integer ranges by whole steps ("0-2" => 0, 1, 2). level=* comes from untrusted OSM
      // tags, so reject absurdly wide ranges (e.g. "0-100000") rather than expanding to 100k doubles.
      double constexpr kMaxRangeFloors = 100.0;
      if (to - from > kMaxRangeFloors)
      {
        failed = true;
        return;
      }
      for (double l = from; l <= to + kLevelEpsilon; l += 1.0)
        AppendUnique(levels, l);
      return;
    }

    failed = true;
  });

  if (failed)
    return {};

  std::sort(levels.begin(), levels.end());
  return levels;
}

bool LevelsContain(std::string_view s, double level)
{
  auto const levels = ParseLevels(s);
  if (levels.empty())
    return std::fabs(level) < kLevelEpsilon;

  for (double const l : levels)
    if (std::fabs(l - level) < kLevelEpsilon)
      return true;
  return false;
}

std::string FormatLevel(double level)
{
  if (std::fabs(level - std::round(level)) < kLevelEpsilon)
    return std::to_string(static_cast<int64_t>(std::llround(level)));

  // Classic locale so a half-level renders as "0.5" regardless of the process locale (never "0,5").
  std::ostringstream ss;
  ss.imbue(std::locale::classic());
  ss << level;
  return ss.str();
}
}  // namespace indoor
