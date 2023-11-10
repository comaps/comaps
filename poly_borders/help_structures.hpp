#pragma once

#include "geometry/rect2d.hpp"

#include "base/non_intersecting_intervals.hpp"

#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace poly_borders
{

struct Link
{
  inline static auto constexpr kInvalidId = std::numeric_limits<size_t>::max();

  Link() = default;
  Link(size_t borderId, size_t pointId) : m_borderId(borderId), m_pointId(pointId) {}

  bool operator<(Link const & rhs) const;

  size_t m_borderId = kInvalidId;
  size_t m_pointId = kInvalidId;
};

/// \note Using next semantic here: [replaceFrom, replaceTo], [replaceFromSrc, replaceToSrc].
struct ReplaceData
{
  ReplaceData(size_t replaceFrom, size_t replaceTo, size_t replaceFromSrc, size_t replaceToSrc,
              size_t borderIdSrc, bool reversed)
    : m_dstFrom(replaceFrom)
    , m_dstTo(replaceTo)
    , m_srcReplaceFrom(replaceFromSrc)
    , m_srcReplaceTo(replaceToSrc)
    , m_srcBorderId(borderIdSrc)
    , m_reversed(reversed) {}

  bool operator<(ReplaceData const & rhs) const;

  size_t m_dstFrom;
  size_t m_dstTo;

  size_t m_srcReplaceFrom;
  size_t m_srcReplaceTo;
  size_t m_srcBorderId;

  // If |m_srcReplaceFrom| was greater than |m_srcReplaceTo|.
  bool m_reversed;
};

struct MarkedPoint
{
  MarkedPoint() = default;
  MarkedPoint(m2::PointD const & point) : m_point(point) {}

  void AddLink(size_t borderId, size_t pointId);

  std::optional<Link> GetLink(size_t curBorderId) const;

  bool EqualDxDy(MarkedPoint const & p, double eps) const
  {
    return m_point.EqualDxDy(p.m_point, eps);
  }

  m2::PointD m_point;
  std::set<Link> m_links;
};

struct Polygon
{
  Polygon() = default;
  Polygon(m2::RectD const & rect, std::vector<m2::PointD> const & points) : m_rect(rect)
  {
    m_points.assign(points.begin(), points.end());
  }
  Polygon(m2::RectD const & rect, std::vector<MarkedPoint> && points)
    : m_rect(rect), m_points(std::move(points))
  {
  }

  Polygon(Polygon &&) = default;
  Polygon & operator=(Polygon &&) noexcept = default;

  // [a, b]
  // @{
  void MakeFrozen(size_t a, size_t b);
  bool IsFrozen(size_t a, size_t b) const;
  // @}

  // [replaceFrom, replaceTo], [replaceFromSrc, replaceToSrc]
  void AddReplaceInfo(size_t replaceFrom, size_t replaceTo,
                      size_t replaceFromSrc, size_t replaceToSrc, size_t borderIdSrc,
                      bool reversed);

  std::set<ReplaceData>::const_iterator FindReplaceData(size_t index);

  m2::RectD m_rect;
  std::vector<MarkedPoint> m_points;
  base::NonIntersectingIntervals<size_t> m_replaced;
  std::set<ReplaceData> m_replaceData;
};
}  // namespace poly_borders
