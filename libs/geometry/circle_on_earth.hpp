#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace ms
{
class LatLon;

std::vector<m2::PointD> CreateCircleGeometryOnEarth(ms::LatLon const & center, double radiusMeters,
                                                    double angleStepDegree);
}  // namespace ms
