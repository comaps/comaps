#pragma once

#include "3party/ankerl/unordered_dense.h"
#include "3party/jansson/jansson/src/jansson.h"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"

#include "boost/container/flat_map.hpp"

#include "indexer/reviews_model.hpp"

#include <string>
#include <vector>

/// Common functionality for parsing reviews.
namespace generator::reviews
{

// exposed for testing
namespace internal
{
using OsmIdToFeatureIdMap = ankerl::unordered_dense::map<base::GeoObjectId, ::reviews::FeatureId>;
using FeatureReviewsMap = boost::container::flat_map<::reviews::FeatureId, ::reviews::FeatureReviews>;

/*!
 * @param json the output JSON from review preprocessor
 * @param osmIdToFeatureId a map of OSM ids to feature ids
 * @param featureToReviews the map to populate with mapping of feature ids to reviews
 */
void ParseReviews(json_t * json, OsmIdToFeatureIdMap const & osmIdToFeatureId, FeatureReviewsMap & featureToReviews);

}  // namespace internal

DECLARE_EXCEPTION(ReviewsParseError, RootException);

/*!
 *
 * \param[in] reviewsFile path to the reviews JSON file
 * \param[in] osmIdToFeatureIdFile path to a file with mapping from OSM id to MSM feature id
 * \param[out] featureIds an ordered vector of feature ids
 * \param[out] reviews a vector of reviews, in the order of \p featureIds
 * \throws ReviewsParseError when an error occurs during JSON parsing
 * \throws RootException (other subclasses) for other errors
 */
void LoadReviews(std::string const & reviewsFile, std::string const & osmIdToFeatureIdFile,
                 std::vector<::reviews::FeatureId> & featureIds, std::vector<::reviews::FeatureReviews> & reviews);

}  // namespace generator::reviews
