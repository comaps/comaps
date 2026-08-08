#include "testing/testing.hpp"

#include "coding/writer.hpp"

#include "indexer/reviews_display.hpp"

using namespace ::reviews;

UNIT_TEST(Reviews_Display_StarRating)
{
  TEST_ALMOST_EQUAL_ULPS(ToStarRating(0), 1.0f, ());
  TEST_ALMOST_EQUAL_ULPS(ToStarRating(25), 2.0f, ());
  TEST_ALMOST_EQUAL_ULPS(ToStarRating(50), 3.0f, ());
  TEST_ALMOST_EQUAL_ULPS(ToStarRating(75), 4.0f, ());
  TEST_ALMOST_EQUAL_ULPS(ToStarRating(100), 5.0f, ());
}