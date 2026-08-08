#include "base/geo_object_id.hpp"
#include "cppjansson/cppjansson.hpp"
#include "testing/testing.hpp"

#include "generator/reviews_section_builder.hpp"

namespace generator_tests
{
using namespace std::chrono;
using namespace generator::reviews;
using namespace ::reviews;
using base::GeoObjectId;
using base::Json;

UNIT_TEST(Reviews_ParseJson)
{
  std::string const reviewsJson = R"(
{
  "relation/123": {
    "average_rating": 2,
    "reviews": [
      {
        "iat": 1777000003,
        "rating": 1,
        "opinion": "opinion without author",
        "author": null
      },
      {
        "iat": 1777000002,
        "rating": 2,
        "opinion": null,
        "author": "author without opinion"
      },
      {
        "iat": 1777000001,
        "rating": 3,
        "opinion": null,
        "author": null
      }
    ]
  },
  "node/12930342733": {
    "average_rating": 83,
    "reviews": [
      {
        "iat": 1776978300,
        "rating": 90,
        "opinion": "Very good coffee.",
        "author": "reviewer 2"
      },
      {
        "iat": 1767297908,
        "rating": 75,
        "opinion": "Okay coffee",
        "author": "reviewer 1"
      }
    ]
  },
  "way/666": {
    "average_rating": 0,
    "reviews": [
      {
        "iat": 1776666666,
        "rating": 0,
        "opinion": "This element does not exist",
        "author": "tester"
      }
    ]
  }
}
)";
  OsmIdToFeatureIdMap osmIdToFeatureId = {};
  osmIdToFeatureId[GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 123)] = 456;
  osmIdToFeatureId[GeoObjectId(GeoObjectId::Type::ObsoleteOsmNode, 12930342733)] = 23456;

  Json const root(reviewsJson.c_str());
  FeatureReviewsMap reviewsByFeatureId;
  ParseReviews(root.get(), osmIdToFeatureId, reviewsByFeatureId);

  TEST_EQUAL(2, reviewsByFeatureId.size(), ());
  {
    constexpr uint32_t expectedKey = 23456;
    TEST(reviewsByFeatureId.contains(expectedKey), ());
    auto const [average_rating, reviews] = reviewsByFeatureId.at(expectedKey);
    TEST_EQUAL(83, (uint16_t)average_rating, ());
    TEST_EQUAL(2, reviews.size(), ());

    TEST_EQUAL(2026y / April / 23, reviews[0].date, ());
    TEST_EQUAL(90, (uint16_t)reviews[0].rating, ());
    TEST_EQUAL("Very good coffee.", reviews[0].opinion, ());
    TEST_EQUAL("reviewer 2", reviews[0].author, ());

    TEST_EQUAL(2026y / January / 1, reviews[1].date, ());
    TEST_EQUAL(75, (uint16_t)reviews[1].rating, ());
    TEST_EQUAL("Okay coffee", reviews[1].opinion, ());
    TEST_EQUAL("reviewer 1", reviews[1].author, ());
  }
  {
    constexpr uint32_t expectedKey = 456;
    TEST(reviewsByFeatureId.contains(expectedKey), ());
    auto const & featureReviews = reviewsByFeatureId.at(expectedKey);
    TEST_EQUAL(3, featureReviews.reviews.size(), ());
  }
}

}  // namespace generator_tests
