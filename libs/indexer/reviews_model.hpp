#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace reviews
{

/*! An integral rating in the range [0..100] */
using Rating = uint8_t;
constexpr Rating MIN_RATING = 0;
constexpr Rating MAX_RATING = 100;

/*! A review of a feature. */
struct Review
{
  /*! in the range 0..100 */
  Rating rating;
  /*! can be in any language */
  std::string opinion;
  /*! the author name as provided by the source system */
  std::string author;
  /*! when the latest version of the review was published */
  std::chrono::year_month_day date;
};

/*! All reviews of a feature. */
struct FeatureReviews
{
  /*! in the range 0..100 */
  Rating averageRating;
  /*! ordered by date, newest first */
  std::vector<Review> reviews;
};

using FeatureId = uint32_t;

}  // namespace reviews
