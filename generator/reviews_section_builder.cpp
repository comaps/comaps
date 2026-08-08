#include "generator/reviews_section_builder.hpp"

#include "base/logging.hpp"
#include "cppjansson/cppjansson.hpp"
#include "defines.hpp"
#include "indexer/reviews_serdes.hpp"
#include "utils.hpp"

namespace generator
{

namespace reviews
{

using namespace ::reviews;

bool LoadOsmIdToFeatureIdMapping(std::string const & osmIdsToFeatureIdsFile, OsmIdToFeatureIdMap & osmIdToFeatureId)
{
  // TODO: what if there are multiple feature ids for an osm id?
  bool const mappedIds =
      ForEachOsmId2FeatureId(osmIdsToFeatureIdsFile, [&osmIdToFeatureId](auto const & compositeId, auto featureId)
  { osmIdToFeatureId[compositeId.m_mainId] = featureId; });
  CHECK(mappedIds, (osmIdsToFeatureIdsFile));
  LOG(LDEBUG, ("loaded", osmIdToFeatureId.size(), " mappings"));
  return true;
}

base::GeoObjectId ParseGeoObjectId(std::string const & id)
{
  using base::GeoObjectId;

  size_t const sep = id.find('/');
  if (std::string::npos == sep)
    MYTHROW(ParseError, ("invalid OSM id", id));
  std::string const typeStr = id.substr(0, sep);

  GeoObjectId::Type type;
  if ("node" == typeStr)
    type = GeoObjectId::Type::ObsoleteOsmNode;
  else if ("relation" == typeStr)
    type = GeoObjectId::Type::ObsoleteOsmRelation;
  else if ("way" == typeStr)
    type = GeoObjectId::Type::ObsoleteOsmWay;
  else
    MYTHROW(ParseError, ("invalid OSM element type", typeStr, "in", id));

  std::string const numStr = id.substr(sep + 1);
  char * end;
  uint64_t const num = strtoull(numStr.c_str(), &end, 10);

  return GeoObjectId(type, num);
}

Rating ParseAverageRating(json_t const * json, char const * osmId)
{
  if (!json_is_integer(json))
    MYTHROW(ParseError, (osmId, ": invalid value type for 'average_rating':", json->type));
  return static_cast<Rating>(json_integer_value(json));
}

Review ParseReview(json_t const * json, char const * osmId, uint const reviewIdx)
{
  if (!json_is_object(json))
    MYTHROW(ParseError, (osmId, reviewIdx, ": invalid value type for review:", json->type));
  Review review;
  {
    json_t const * const val = json_object_get(json, "iat");
    if (!json_is_integer(val))
      MYTHROW(ParseError, (osmId, reviewIdx, ": invalid value type for 'iat':", val->type));
    using namespace std::chrono;
    review.date = year_month_day{floor<days>(local_seconds{seconds{json_integer_value(val)}})};
  }
  {
    json_t const * const val = json_object_get(json, "rating");
    if (!json_is_integer(val))
      MYTHROW(ParseError, (osmId, reviewIdx, ": invalid value type for 'rating':", val->type));
    review.rating = static_cast<Rating>(json_integer_value(val));
  }
  {
    json_t const * const val = json_object_get(json, "opinion");
    if (json_is_null(val))
      review.opinion = "";
    else if (json_is_string(val))
      review.opinion = static_cast<std::string>(json_string_value(val));
    else
      MYTHROW(ParseError, (osmId, reviewIdx, ": invalid value type for 'opinion':", val->type));
  }
  {
    json_t const * const val = json_object_get(json, "author");
    if (json_is_null(val))
      review.author = "";
    else if (json_is_string(val))
      review.author = static_cast<std::string>(json_string_value(val));
    else
      MYTHROW(ParseError, (osmId, reviewIdx, ": invalid value type for 'author':", val->type));

  }
  return review;
}

std::vector<Review> ParseReviewsArray(json_t const * json, char const * osmId)
{
  if (!json_is_array(json))
    MYTHROW(ParseError, (osmId, ": invalid value type for 'reviews':", json->type));

  std::vector<Review> reviews;
  {
    uint i;
    json_t * reviewJson;
    json_array_foreach(json, i, reviewJson)
    {
      reviews.push_back(ParseReview(reviewJson, osmId, i));
    }
  }
  return reviews;
}

void ParseReviews(json_t * json, OsmIdToFeatureIdMap const & osmIdToFeatureId, FeatureReviewsMap & featureToReviews)
{
  if (json == nullptr || !json_is_object(json))
    MYTHROW(ParseError, ("required JSON object, got", json->type));

  for (void * it = json_object_iter(json); it; it = json_object_iter_next(json, it))
  {
    char const * osmIdStr = json_object_iter_key(it);
    json_t const * featureReviewsJson = json_object_iter_value(it);
    auto const osmId = ParseGeoObjectId(osmIdStr);
    if (!osmIdToFeatureId.contains(osmId))
    {
      // it is expected that some reviewed place ids will not exist in the id map:
      // 1. the map only covers the current MWM file while the reviews JSON is global;
      // 2. there is no guarantee the JSON is perfectly in sync with the source OSM data.
      LOG(LDEBUG, ("unmatched", osmId));
      continue;
    }
    auto const featureId = osmIdToFeatureId.at(osmId);

    featureToReviews[featureId] = FeatureReviews{
        ParseAverageRating(json_object_get(featureReviewsJson, "average_rating"), osmIdStr),
        ParseReviewsArray(json_object_get(featureReviewsJson, "reviews"), osmIdStr),
    };
  }
}

/*!
 *
 * \param[in] reviewsFile path to the reviews JSON file
 * \param[in] osmIdToFeatureId a map from OSM id to MSM feature id
 * \param[out] featureIds an ordered vector of feature ids
 * \param[out] reviews a vector of reviews, in the order of \p featureIds
 * \throws ParseError when an error occurs during JSON parsing
 * \throws RootException (other subclasses) for other errors
 */
void LoadReviews(std::string const & reviewsFile, OsmIdToFeatureIdMap const & osmIdToFeatureId,
                 std::vector<FeatureId> & featureIds, std::vector<FeatureReviews> & reviews)
{
  FeatureReviewsMap reviewsByFeatureId;

  std::string jsonBuffer;
  GetPlatform().GetReader(reviewsFile)->ReadAsString(jsonBuffer);

  base::Json const root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", reviewsFile));
  ParseReviews(root.get(), osmIdToFeatureId, reviewsByFeatureId);

  for (auto const & [featureId, featureReviews] : reviewsByFeatureId)
  {
    featureIds.push_back(featureId);
    reviews.push_back(featureReviews);
  }

  LOG(LINFO, ("Found reviews for", featureIds.size(), "features"));
}

}  // namespace reviews

void BuildReviewsSection(std::string const & reviewsFile, std::string const & mwmFile)
{
  using namespace reviews;

  LOG(LINFO, ("Populating", REVIEWS_FILE_TAG, "section from", reviewsFile));
  OsmIdToFeatureIdMap osmIdToFeatureId;

  if (std::string const osmIdsToFeatureIdsFile = mwmFile + OSM2FEATURE_FILE_EXTENSION;
      !LoadOsmIdToFeatureIdMapping(osmIdsToFeatureIdsFile, osmIdToFeatureId))
  {
    LOG(LERROR, ("Error parsing mapping file ", osmIdsToFeatureIdsFile));
    return;
  }

  std::vector<FeatureId> featureIds;
  std::vector<FeatureReviews> reviews;
  try
  {
    LoadReviews(reviewsFile, osmIdToFeatureId, featureIds, reviews);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error loading reviews from", reviewsFile, e.Msg()));
    return;
  }

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  auto const writer = cont.GetWriter(REVIEWS_FILE_TAG);
  uint64_t sectionSize = writer->Pos();
  Serialize(*writer, featureIds, reviews);
  sectionSize = writer->Pos() - sectionSize;

  LOG(LINFO, ("Section", REVIEWS_FILE_TAG, "is built in", mwmFile, "Disk size =", sectionSize, "bytes"));
}
}  // namespace generator
