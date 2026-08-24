#pragma once

#include "indexer/indoor.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <optional>

namespace indoor
{
using FeatureFn = std::function<void(FeatureView const &)>;
using ForEachFn = std::function<void(m2::RectD const &, FeatureFn const &)>;

/**
 * Finds the complex to show, or nullopt. Holds no state and touches no threads, so tests can drive
 * it with plain FeatureViews instead of a generated MWM.
 * @param center where the user is looking, so a building at the screen edge never activates
 * @param current the complex already showing, kept while the center stays inside it so the
 *        footprint does not flicker as the map pans
 */
std::optional<Complex> ScanForActiveComplex(m2::PointD const & center, ForEachFn const & forEach,
                                            Complex const * current);
}  // namespace indoor
