#include "testing/testing.hpp"

#include "coding/writer.hpp"

#include "indexer/reviews_model.hpp"
#include "indexer/reviews_serdes.hpp"

using namespace ::reviews;

UNIT_TEST(Reviews_SerDes)
{
  using namespace std::chrono;
  std::vector<FeatureId> const features = {100, 105, 200};
  std::vector<FeatureReviews> const reviews = {{12,
                                                {
                                                    {12, "bad review", "reviewer 1", 2026y / April / 23},
                                                }},
                                               {23,
                                                {
                                                    {22, "okay", "reviewer 1", 2026y / March / 22},
                                                    {24, "", "reviewer 2", 2026y / March / 21},
                                                }},
                                               {34,
                                                {
                                                    {36, "nice sandwich", "reviewer 3", 2025y / April / 23},
                                                    {32, "very busy place", "reviewer 2", 2025y / April / 22},
                                                    {34, "", "", 2000y / April / 23},
                                                }}};

  {
    std::vector<uint8_t> buffer;
    {
      MemWriter writer(buffer);
      Serialize(writer, features, reviews);
    }

    MemReader reader(buffer.data(), buffer.size());
    Deserializer des;

    // comprehensive checks for the first feature
    {
      std::optional<FeatureReviews> const fopt = des.Deserialize(reader, 100);
      TEST(fopt.has_value(), ());
      auto const & f = fopt.value();
      TEST_EQUAL(f.averageRating, 12, ());
      TEST_EQUAL(f.reviews.size(), 1, ());
      TEST_EQUAL(f.reviews[0].rating, 12, ());
      TEST_EQUAL(f.reviews[0].opinion, "bad review", ());
      TEST_EQUAL(f.reviews[0].author, "reviewer 1", ());
      TEST_EQUAL(f.reviews[0].date, 2026y / April / 23, ());
    }

    // spot checks for the other features
    {
      std::optional<FeatureReviews> const fopt = des.Deserialize(reader, 105);
      TEST(fopt.has_value(), ());
      auto const & f = fopt.value();
      TEST_EQUAL(f.reviews.size(), 2, ());
      TEST_EQUAL(f.reviews[1].rating, 24, ());
      TEST(f.reviews[1].opinion.empty(), ());
    }
    {
      std::optional<FeatureReviews> const fopt = des.Deserialize(reader, 200);
      TEST(fopt.has_value(), ());
      auto const & f = fopt.value();
      TEST_EQUAL(f.reviews.size(), 3, ());
      TEST_EQUAL(f.reviews[1].opinion, "very busy place", ());
      TEST_EQUAL(f.reviews[1].author, "reviewer 2", ());
      TEST(f.reviews[2].author.empty(), ());
      TEST_EQUAL(f.reviews[2].date, 2000y / April / 23, ());
    }

    // missing feature should have no value
    {
      std::optional<FeatureReviews> const fopt = des.Deserialize(reader, 123);
      TEST(!fopt.has_value(), ());
    }
  }
}
