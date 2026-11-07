#pragma once

#include "storage/storage_defines.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define BORDERS_DIR       "borders/"
#define BORDERS_EXTENSION ".poly"

namespace borders
{
// The raw borders that we have somehow obtained (probably downloaded from
// the OSM and then manually tweaked, but the exact knowledge is lost) are
// stored in the BORDERS_DIR.
// These are the sources of the mwm borders: each file there corresponds
// to exactly one mwm and all mwms except for World and WorldCoasts must
// have a borders file to be generated from.
//
// The file format for raw borders is described at
//   https://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format
//
// The borders for all mwm files are shipped with the application in
// the mwm binary format for geometry data (see coding/geometry_coding.hpp).
// However, storing every single point turned out to take too much space,
// therefore the borders are simplified. This simplification may lead to
// unwanted consequences (for example, empty spaces may occur between mwms)
// but currently we do not take any action against them.

using Polygon = m2::RegionD;
using PolygonsTree = m4::Tree<Polygon>;

class CountryPolygons
{
public:
  CountryPolygons() = default;
  explicit CountryPolygons(std::string && name, PolygonsTree && regions)
    : m_name(std::move(name))
    , m_polygons(std::move(regions))
  {}

  std::string const & GetName() const { return m_name; }
  bool IsEmpty() const { return m_polygons.IsEmpty(); }
  void Clear()
  {
    m_polygons.Clear();
    m_name.clear();
  }

  class ContainsCompareFn
  {
    double m_eps, m_squareEps;

  public:
    explicit ContainsCompareFn(double eps) : m_eps(eps), m_squareEps(eps * eps) {}
    bool EqualPoints(m2::PointD const & p1, m2::PointD const & p2) const
    {
      return AlmostEqualAbs(p1.x, p2.x, m_eps) && AlmostEqualAbs(p1.y, p2.y, m_eps);
    }
    bool EqualZeroSquarePrecision(double val) const { return AlmostEqualAbs(val, 0.0, m_squareEps); }
  };

  static double GetContainsEpsilon() { return 1.0E-4; }

  bool Contains(m2::PointD const & point) const;

  template <typename Do>
  void ForEachPolygon(Do && fn) const
  {
    m_polygons.ForEach(std::forward<Do>(fn));
  }

  template <typename Do>
  bool ForAnyPolygon(Do && fn) const
  {
    return m_polygons.ForAny(std::forward<Do>(fn));
  }

private:
  std::string m_name;

  /// @todo Is it an overkill to store Tree4D for each country's polygon?
  PolygonsTree m_polygons;
};

class CountryPolygonsCollection
{
public:
  CountryPolygonsCollection() = default;

  void Add(CountryPolygons && countryPolygons)
  {
    auto const res = m_countryPolygonsMap.emplace(countryPolygons.GetName(), std::move(countryPolygons));
    CHECK(res.second, ());

    auto const & inserted = res.first->second;
    inserted.ForEachPolygon([&inserted, this](Polygon const & polygon)
    { m_regionsTree.Add(inserted, polygon.GetRect()); });
  }

  size_t GetSize() const { return m_countryPolygonsMap.size(); }

  template <typename ToDo>
  void ForEachCountryInRect(m2::RectD const & rect, ToDo && toDo) const
  {
    std::unordered_set<CountryPolygons const *> uniq;
    m_regionsTree.ForEachInRect(rect, [&](CountryPolygons const & cp)
    {
      if (uniq.insert(&cp).second)
        toDo(cp);
    });
  }

  bool HasRegionByName(std::string const & name) const { return m_countryPolygonsMap.count(name) != 0; }

  CountryPolygons const & GetRegionByName(std::string const & name) const
  {
    ASSERT(HasRegionByName(name), ());

    return m_countryPolygonsMap.at(name);
  }

private:
  m4::Tree<std::reference_wrapper<CountryPolygons const>> m_regionsTree;
  std::unordered_map<std::string, CountryPolygons> m_countryPolygonsMap;
};

using PolygonsList = std::vector<Polygon>;

/// @return false if borderFile can't be opened
bool LoadBorders(std::string const & borderFile, PolygonsList & outBorders);

bool GetBordersRect(std::string const & baseDir, std::string const & country, m2::RectD & bordersRect);

bool LoadCountriesList(std::string const & baseDir, CountryPolygonsCollection & countries);

void GeneratePackedBorders(std::string const & baseDir);

template <typename Source>
PolygonsList ReadPolygonsOfOneBorder(Source & src)
{
  auto const count = ReadVarUint<uint32_t>(src);
  PolygonsList result(count);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    result[i] = m2::RegionD(std::move(points));
  }

  return result;
}

void DumpBorderToPolyFile(std::string const & filePath, storage::CountryId const & mwmName,
                          PolygonsList const & polygons);
void UnpackBorders(std::string const & baseDir, std::string const & targetDir);

CountryPolygonsCollection const & GetOrCreateCountryPolygonsTree(std::string const & baseDir);
}  // namespace borders
