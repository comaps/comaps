#pragma once

#include "indexer/reviews_model.hpp"

namespace reviews
{

/*! A fractional star rating in the range [1..5] */
using StarRating = float;
constexpr StarRating MIN_STAR_RATING = 1.0;
constexpr StarRating MAX_STAR_RATING = 5.0;
constexpr auto STAR_RATING_SCALE = (MAX_STAR_RATING - MIN_STAR_RATING) / (MAX_RATING - MIN_RATING);

inline StarRating ToStarRating(Rating const rating)
{
  return static_cast<float>(rating) * STAR_RATING_SCALE + MIN_STAR_RATING;
}

}  // namespace reviews
