#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <string_view>
#include <vector>

namespace df
{
// True if feature should be hidden.
//
// Indoor types hidden if inactive.
// Leveled POIs skipped near mismatch.
// Building shell always visible.
//
// |indoorPolygonRects| must already be expanded by the desired proximity margin (see
// indoor::ExpandRectsByMeters); this function only tests rect overlap. |featureRect| should be
// the feature's own bounding rect (not just its center) so that a large non-indoor feature whose
// center is far from an indoor polygon, but whose edge nearly touches it, still counts as near.
bool ShouldSkipIndoorFeature(feature::TypesHolder const & types, std::string_view levelMeta, double activeLevel,
                             m2::RectD const & featureRect, std::vector<m2::RectD> const & indoorPolygonRects,
                             std::vector<double> const & availableLevels);
}  // namespace df
