#pragma once

#include "poly_borders/help_structures.hpp"

#include "base/control_flow.hpp"

#include <map>
#include <string>
#include <vector>

namespace poly_borders
{
/// \note All methods of this class are not thread-safe except |MarkPoint()| method.
class BordersData
{
public:
  inline static double const kEqualityEpsilon = 1.0E-7;
  inline static std::string const kBorderExtension = ".poly";

  void Init(std::string const & bordersDir);

  void RemoveEmptySpaceBetweenBorders();

  void DumpPolyFiles(std::string const & targetDir);
  Polygon const & GetBordersPolygonByName(std::string const & name) const;
  void PrintDiff();

private:
  /// \brief Some polygons can have sequentially same points - duplicates. This method removes such
  /// points and leaves only unique.
  size_t RemoveDuplicatePoints();

  template <class PointsT> static size_t RemoveDuplicatingPointImpl(PointsT & points)
  {
    auto const equalFn = [](auto const & p1, auto const & p2)
    {
      return p1.EqualDxDy(p2, kEqualityEpsilon);
    };

    auto const last = std::unique(points.begin(), points.end(), equalFn);
    size_t count = std::distance(last, points.end());
    points.erase(last, points.end());

    while (points.size() > 1 && equalFn(points.front(), points.back()))
    {
      ++count;
      points.pop_back();
    }

    return count;
  }

  /// \brief Checks whether we can replace points from segment: [curLeftPointId, curRightPointId]
  /// of |curBorderId| to points from another border in order to get rid of empty space
  /// between curBorder and anotherBorder.
  base::ControlFlow TryToReplace(size_t curBorderId, size_t & curLeftPointId,
                                 size_t curRightPointId);

  bool HasLinkAt(size_t curBorderId, size_t pointId);

  /// \brief Replace points using |Polygon::ReplaceData| that is filled by
  /// |RemoveEmptySpaceBetweenBorders()|.
  void DoReplace();

  size_t m_removedPointsCount = 0;
  size_t m_duplicatedPointsCount = 0;
  std::map<size_t, double> m_additionalAreaMetersSqr;

  std::map<std::string, size_t> m_polyFileNameToIndex;
  std::map<size_t, std::string> m_indexToPolyFileName;
  std::vector<Polygon> m_bordersPolygons;
  std::vector<Polygon> m_prevCopy;
};
}  // namespace poly_borders
