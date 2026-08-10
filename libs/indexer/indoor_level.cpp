#include "indexer/indoor_level.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "platform/measurement_utils.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace indoor
{
bool IsLevelSensitiveType(feature::TypesHolder const & types)
{
  return !ftypes::IsBuildingChecker::Instance()(types) && !ftypes::IsBuildingPartChecker::Instance()(types);
}

namespace
{
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
    if (LevelsEqual(existing, level))
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
    if (LevelsEqual(l, level))
      return true;
  return false;
}

std::string FormatLevel(double level)
{
  if (LevelsEqual(level, std::round(level)))
    return measurement_utils::ToStringPrecision(level, 0);

  return measurement_utils::ToStringPrecision(level, 2);
}

std::vector<m2::RectD> ExpandRectsByMeters(std::vector<m2::RectD> const & rects, double meters)
{
  std::vector<m2::RectD> result;
  result.reserve(rects.size());
  for (auto const & r : rects)
  {
    // GetSmPoint applies the cos(latitude) correction at each corner's own latitude, so the
    // expansion stays accurate near the poles instead of assuming an equator-flat degree offset.
    m2::PointD const lo = mercator::GetSmPoint(r.LeftBottom(), -meters, -meters);
    m2::PointD const hi = mercator::GetSmPoint(r.RightTop(), meters, meters);
    result.emplace_back(lo.x, lo.y, hi.x, hi.y);
  }
  return result;
}

std::vector<m2::RectD> MergeOverlappingRects(std::vector<m2::RectD> const & rects)
{
  std::vector<m2::RectD> merged(rects.begin(), rects.end());

  // A building mapped room-by-room can produce one rect per room; ShouldSkipIndoorFeature checks
  // every leveled feature against every entry in this list on every re-tile, so collapsing
  // touching/overlapping rects here (once, per background scan) keeps that list small instead of
  // scaling with room count.
  bool changed = true;
  while (changed)
  {
    changed = false;
    for (size_t i = 0; i < merged.size() && !changed; ++i)
    {
      for (size_t j = i + 1; j < merged.size(); ++j)
      {
        if (merged[i].IsIntersect(merged[j]))
        {
          merged[i].Add(merged[j]);
          merged.erase(merged.begin() + j);
          changed = true;
          break;
        }
      }
    }
  }
  return merged;
}
}  // namespace indoor
