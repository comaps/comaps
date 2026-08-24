#include "map/indoor_scan.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cmath>

namespace indoor
{
namespace
{
m2::RectD ProbeRect(m2::PointD const & center)
{
  return mercator::RectByCenterXYAndSizeInMeters(center, kActivationBufferMeters);
}

double PolygonArea(std::vector<m2::TriangleD> const & triangles)
{
  double area = 0.0;
  for (auto const & t : triangles)
    area += std::fabs((t.p2().x - t.p1().x) * (t.p3().y - t.p1().y) -
                      (t.p3().x - t.p1().x) * (t.p2().y - t.p1().y)) / 2.0;
  return area;
}

double SquareMeters(double area, m2::PointD const & at)
{
  double const perDegree = mercator::DistanceOnEarth(at, m2::PointD(at.x + 0.001, at.y)) / 0.001;
  return area * perDegree * perDegree;
}

struct Candidate
{
  Complex m_complex;
  double m_area = 0.0;
  // Stacked over the ground, like a roof.
  bool m_stacked = false;
};

// Buildings whose footprint indoor rooms cover. A building:part is never a candidate.
std::vector<Candidate> FindIndoorBuildings(m2::PointD const & center, ForEachFn const & forEach)
{
  std::vector<Candidate> found;
  std::vector<m2::RectD> indoorAreas;

  forEach(mercator::RectByCenterXYAndSizeInMeters(center, kSearchMeters), [&](FeatureView const & f)
  {
    // A station whose inside is mapped only as levelled platforms still has an inside.
    if (f.m_isArea && (f.m_isIndoor || (f.m_isPlatform && f.m_isLeveled)))
      indoorAreas.push_back(f.m_rect);

    if (!f.m_isBuilding || !f.m_isArea || f.m_isPart)
      return;
    if (f.m_rect.SizeX() > kMaxBuildingRectDeg || f.m_rect.SizeY() > kMaxBuildingRectDeg)
      return;

    double const area = PolygonArea(f.m_triangles);
    if (area <= 0.0)
      return;

    // A roof shapes the footprint but must never be a kept outline, or it hides the floor below.
    bool const stacked = f.m_layer > 0;
    std::vector<FeatureID> members;
    if (!stacked)
      members.push_back(f.m_id);
    found.push_back({Complex{f.m_id, f.m_rect, f.m_triangles, {}, std::move(members)}, area, stacked});
  });

  std::vector<Candidate> result;
  for (auto & candidate : found)
  {
    double covered = 0.0;
    for (auto const & room : indoorAreas)
      if (candidate.m_complex.Owns(room))
        covered += room.SizeX() * room.SizeY();

    if (covered > 0.0 && covered >= kMinIndoorCoverage * candidate.m_area)
      result.push_back(std::move(candidate));
  }
  return result;
}

// Largest, not smallest, so a complex resolves to its shell rather than a polygon under the center.
std::optional<Complex> FindSeed(m2::PointD const & center, std::vector<Candidate> const & pool)
{
  std::optional<Complex> best;
  double bestArea = 0.0;
  m2::RectD const probe = ProbeRect(center);

  for (auto const & candidate : pool)
  {
    if (!candidate.m_complex.Contains(center) && !candidate.m_complex.Reaches(probe))
      continue;
    if (best && candidate.m_area <= bestArea)
      continue;

    best = candidate.m_complex;
    bestArea = candidate.m_area;
  }
  return best;
}

// Merges overlapping indoor buildings, so the complex cannot chain across ordinary ones.
void Glob(Complex & region, std::vector<Candidate> const & pool)
{
  std::vector<bool> taken(pool.size(), false);
  for (size_t i = 0; i < pool.size(); ++i)
    taken[i] = pool[i].m_complex.m_id == region.m_id;

  for (bool grew = true; grew;)
  {
    grew = false;
    for (size_t i = 0; i < pool.size(); ++i)
    {
      auto const & other = pool[i].m_complex;
      if (taken[i] || !region.OverlapsPolygon(other.m_triangles))
        continue;

      taken[i] = true;
      grew = true;
      region.m_rect.Add(other.m_rect);
      region.m_triangles.insert(region.m_triangles.end(), other.m_triangles.begin(), other.m_triangles.end());
      if (!pool[i].m_stacked)
        region.m_members.push_back(other.m_id);
    }
  }
}

// An inside can reach past its outlines, e.g. a platform running beyond the building above it.
void AbsorbIndoorGeometry(Complex & region, ForEachFn const & forEach)
{
  std::vector<FeatureView> absorbed;
  forEach(region.m_rect, [&](FeatureView const & f)
  {
    bool const isInside = f.m_isIndoor || (f.m_isPlatform && f.m_isLeveled);
    // Overlap rather than ownership, because the whole point is to reach the parts hanging outside.
    if (isInside && f.m_isArea && !f.m_triangles.empty() && region.OverlapsPolygon(f.m_triangles) &&
        !region.ContainsPolygon(f.m_triangles))
      absorbed.push_back(f);
  });

  for (auto const & f : absorbed)
  {
    region.m_rect.Add(f.m_rect);
    region.m_triangles.insert(region.m_triangles.end(), f.m_triangles.begin(), f.m_triangles.end());
  }
}

// Levels of indoor content the complex owns.
std::vector<double> CollectLevels(Complex const & region, ForEachFn const & forEach)
{
  std::vector<double> levels;

  forEach(region.m_rect, [&](FeatureView const & f)
  {
    // The same predicate Active::Hides uses, so every floor it can hide is one the picker offers.
    if (!f.NamesAFloor())
      return;

    auto const parsed = ParseLevels(f.m_level);
    if (parsed.empty())
      return;  // No usable level, so it names no floor. Never assume the ground floor.

    if (!region.Owns(f.m_rect))
      return;

    for (double const level : parsed)
      if (std::find(levels.begin(), levels.end(), level) == levels.end())
        levels.push_back(level);
  });

  std::sort(levels.begin(), levels.end());
  return levels;
}
}  // namespace

std::optional<Complex> ScanForActiveComplex(m2::PointD const & center, ForEachFn const & forEach,
                                            Complex const * current)
{
  std::optional<Complex> region;
  // Stay on the current complex while the center is still within reach of it.
  if (current != nullptr && (current->Contains(center) || current->Reaches(ProbeRect(center))))
  {
    region = *current;  // Same complex, so keep its footprint rather than re-deriving it.
  }
  else
  {
    auto const pool = FindIndoorBuildings(center, forEach);
    region = FindSeed(center, pool);
    if (region)
      Glob(*region, pool);
  }

  if (!region)
    return std::nullopt;

  AbsorbIndoorGeometry(*region, forEach);

  // A closet is not somewhere you walk around, and activating on one hides every room nearby.
  if (SquareMeters(PolygonArea(region->m_triangles), center) < kMinComplexAreaM2)
    return std::nullopt;

  region->m_levels = CollectLevels(*region, forEach);
  if (region->m_levels.size() < kMinLevels)
    return std::nullopt;

  return region;
}
}  // namespace indoor
