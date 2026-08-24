#include "indexer/indoor.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace indoor
{
FeatureView MakeFeatureView(FeatureType & ft, feature::TypesHolder const & types, int scale, bool withGeometry)
{
  FeatureView f;
  f.m_id = ft.GetID();
  f.m_isArea = ft.GetGeomType() == feature::GeomType::Area;
  f.m_isPart = ftypes::IsBuildingPartChecker::Instance()(types);
  f.m_isBuilding = f.m_isPart || ftypes::IsBuildingChecker::Instance()(types);
  f.m_isIndoor = ftypes::IsIndoorChecker::Instance()(types);
  f.m_isPlatform = ftypes::IsPlatformChecker::Instance()(types);
  f.m_level = ft.GetMetadata(feature::Metadata::FMD_LEVEL);
  f.m_isLeveled = !f.m_level.empty();
  f.m_layer = ft.GetLayer();
  f.m_rect = ft.GetLimitRect(scale);

  // Reading shapes is slow, so only do it for the buildings, rooms and platforms that form a complex.
  if (withGeometry && f.m_isArea && (f.m_isBuilding || f.m_isIndoor || f.m_isPlatform))
    ft.ForEachTriangle([&f](m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
    { f.m_triangles.emplace_back(a, b, c); }, scale);

  return f;
}

bool Complex::IsMember(FeatureID const & id) const
{
  return std::find(m_members.begin(), m_members.end(), id) != m_members.end();
}

bool Complex::Contains(m2::PointD const & pt) const
{
  return m_rect.IsPointInside(pt) && m2::IsPointInsideTriangles(pt, m_triangles);
}

bool Complex::ContainsPolygon(std::vector<m2::TriangleD> const & triangles) const
{
  for (auto const & t : triangles)
    if (!Contains(t.p1()) || !Contains(t.p2()) || !Contains(t.p3()))
      return false;
  return true;
}

bool Complex::OverlapsPolygon(std::vector<m2::TriangleD> const & triangles) const
{
  if (triangles.empty())
    return false;

  m2::RectD other;
  for (auto const & t : triangles)
  {
    other.Add(t.p1());
    other.Add(t.p2());
    other.Add(t.p3());
  }
  if (!m_rect.IsIntersect(other))
    return false;

  for (auto const & t : triangles)
    if (Contains(t.p1()) || Contains(t.p2()) || Contains(t.p3()))
      return true;

  // Test each shape against the other, or one sitting wholly inside the other reads as no overlap.
  for (auto const & mine : m_triangles)
  {
    // Hundreds of triangles here, so reject on the box before testing any point against it.
    m2::RectD box;
    box.Add(mine.p1());
    box.Add(mine.p2());
    box.Add(mine.p3());
    if (!box.IsIntersect(other))
      continue;

    for (auto const & t : triangles)
      if (m2::IsPointInsideTriangle(mine.p1(), t.p1(), t.p2(), t.p3()) ||
          m2::IsPointInsideTriangle(mine.p2(), t.p1(), t.p2(), t.p3()) ||
          m2::IsPointInsideTriangle(mine.p3(), t.p1(), t.p2(), t.p3()))
        return true;
  }

  return false;
}

bool Complex::Owns(m2::RectD const & rect) const
{
  return Contains(rect.Center());
}

bool Complex::Reaches(m2::RectD const & probe) const
{
  if (!m_rect.IsIntersect(probe))
    return false;
  return std::any_of(m_triangles.begin(), m_triangles.end(), [&probe](m2::TriangleD const & t)
  {
    // Hundreds of triangles per ordinary feature in a tile, so reject on the box first.
    m2::RectD box;
    box.Add(t.p1());
    box.Add(t.p2());
    box.Add(t.p3());
    if (!box.IsIntersect(probe))
      return false;
    return m2::TriangleIntersectsRect(probe, t.p1(), t.p2(), t.p3());
  });
}

bool Active::Hides(FeatureView const & f) const
{
  // Rooms and walls stay invisible until you are actually inside a building, never drawn as scenery.
  if (m_complex == nullptr)
    return f.m_isIndoor;

  // Keep the outlines the complex was built from and hide the roofs and parts stacked on top of them.
  if (f.m_isBuilding && !f.m_isIndoor)
    return m_complex->Owns(f.m_rect) && !m_complex->IsMember(f.m_id);

  if (!f.NamesAFloor())
    return false;

  // Indoor content belongs to a building, everything else merely covers it.
  bool const mine = f.m_isIndoor ? m_complex->Owns(f.m_rect) : m_complex->Reaches(f.m_rect);
  if (!mine)
    return f.m_isIndoor;

  return !LevelsContain(f.m_level, m_level);
}

bool Active::Flattens(std::vector<m2::TriangleD> const & triangles) const
{
  return m_complex != nullptr && m_complex->OverlapsPolygon(triangles);
}

bool Active::IgnoresLayer(FeatureView const & f) const
{
  if (m_complex == nullptr || (!f.m_isIndoor && !f.m_isLeveled))
    return false;

  // Match how Hides picks an owner, so everything a floor shows is also stacked with that floor.
  return f.m_isIndoor ? m_complex->Owns(f.m_rect) : m_complex->Reaches(f.m_rect);
}

bool Active::Sinks(FeatureView const & f) const
{
  // Nothing sinks on the ground floor because outdoor streets are level with indoor level 0.
  if (m_complex == nullptr || LevelsEqual(m_level, 0.0))
    return false;

  // Buildings have their own rule already, and anything carrying a level is judged by that level.
  if (f.m_isIndoor || f.m_isLeveled || f.m_isBuilding)
    return false;

  return m_complex->Reaches(f.m_rect);
}

bool Active::DrawsAsFloor(FeatureView const & f) const
{
  return m_complex != nullptr && f.m_isArea && f.m_isPlatform && f.m_isLeveled &&
         m_complex->Reaches(f.m_rect);
}

double Normalize(double level)
{
  return std::round(level / kLevelStep) * kLevelStep;
}

namespace
{
bool InRange(double level)
{
  return std::isfinite(level) && std::fabs(level) <= kMaxLevel;
}

void AppendUnique(std::vector<double> & levels, double level)
{
  level = Normalize(level);
  if (std::find(levels.begin(), levels.end(), level) == levels.end())
    levels.push_back(level);
}

// Splits "0-2" or "-2--1" into endpoints. An interior '-' separates, a leading '-' is a sign.
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
}  // namespace

std::vector<double> ParseLevels(std::string_view s)
{
  std::vector<double> levels;
  bool failed = false;

  strings::Tokenize(s, ";", [&](std::string_view token)
  {
    if (failed)
      return;

    strings::Trim(token);
    if (token.empty())
      return;

    double level;
    if (strings::to_double(token, level))
    {
      if (InRange(level))
        AppendUnique(levels, level);
      else
        failed = true;
      return;
    }

    double from, to;
    if (ParseRange(token, from, to) && InRange(from) && InRange(to))
    {
      // Ensure a fractional level is not dropped.
      double const first = Normalize(from), last = Normalize(to);
      if ((last - first) / kLevelStep > 2 * kMaxLevel)
      {
        failed = true;
        return;
      }
      for (double l = first; l <= last + kLevelStep / 2; l += kLevelStep)
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
  double const target = Normalize(level);
  return std::find(levels.begin(), levels.end(), target) != levels.end();
}

std::string FormatLevel(double level)
{
  level = Normalize(level);
  if (level == std::round(level))
    return std::to_string(static_cast<int64_t>(level));

  std::ostringstream ss;
  ss.imbue(std::locale::classic());
  ss << level;
  return ss.str();
}
}  // namespace indoor
