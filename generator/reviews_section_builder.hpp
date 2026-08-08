#pragma once

#include "3party/ankerl/unordered_dense.h"
#include "3party/jansson/jansson/src/jansson.h"
#include "base/exception.hpp"
#include "base/geo_object_id.hpp"
#include "boost/container/flat_map.hpp"
#include "indexer/reviews_model.hpp"

#include <string>

namespace generator
{

/// the interface in this namespace is exposed for testing
namespace reviews
{
DECLARE_EXCEPTION(ParseError, RootException);

using OsmIdToFeatureIdMap = ankerl::unordered_dense::map<base::GeoObjectId, ::reviews::FeatureId>;
using FeatureReviewsMap = boost::container::flat_map<::reviews::FeatureId, ::reviews::FeatureReviews>;

void ParseReviews(json_t * json, OsmIdToFeatureIdMap const & osmIdToFeatureId, FeatureReviewsMap & featureToReviews);

}  // namespace reviews

/*!
 * \brief Write a reviews section to the specfied MWM file.
 * \param reviewsFile a path to reviews JSON file output by the reviews preprocessing tool
 * \param mwmFile a path to the MWM file being updated
 */
void BuildReviewsSection(std::string const & reviewsFile, std::string const & mwmFile);
}  // namespace generator
