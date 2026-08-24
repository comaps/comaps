#include "testing/testing.hpp"

#include "indexer/indoor.hpp"

#include <vector>

namespace indoor_level_tests
{
using Levels = std::vector<double>;

UNIT_TEST(IndoorLevel_ParseSingle)
{
  TEST_EQUAL(indoor::ParseLevels("0"), Levels({0.0}), ());
  TEST_EQUAL(indoor::ParseLevels("-1"), Levels({-1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("1.5"), Levels({1.5}), ());
  TEST_EQUAL(indoor::ParseLevels(" 2 "), Levels({2.0}), ());
}

UNIT_TEST(IndoorLevel_ParseLists)
{
  TEST_EQUAL(indoor::ParseLevels("0;1;2"), Levels({0.0, 1.0, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("2;0;1"), Levels({0.0, 1.0, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("0;0;1"), Levels({0.0, 1.0}), ());
}

UNIT_TEST(IndoorLevel_ParseRanges)
{
  TEST_EQUAL(indoor::ParseLevels("0-2"), Levels({0.0, 0.5, 1.0, 1.5, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("-2--1"), Levels({-2.0, -1.5, -1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("0.5-2"), Levels({0.5, 1.0, 1.5, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("0-2.5"), Levels({0.0, 0.5, 1.0, 1.5, 2.0, 2.5}), ());
}

UNIT_TEST(IndoorLevel_Normalize)
{
  TEST_EQUAL(indoor::ParseLevels("-1.3;-1.7"), Levels({-1.5}), ());
  TEST_EQUAL(indoor::ParseLevels("1.3"), Levels({1.5}), ());
  TEST_EQUAL(indoor::ParseLevels("1.2499;1.2501"), Levels({1.0, 1.5}), ());
  TEST_EQUAL(indoor::ParseLevels("2;2.5"), Levels({2.0, 2.5}), ());
}

UNIT_TEST(IndoorLevel_ParseInvalid)
{
  TEST(indoor::ParseLevels("").empty(), ());
  TEST(indoor::ParseLevels("G").empty(), ());
  TEST(indoor::ParseLevels("ground").empty(), ());
  TEST(indoor::ParseLevels("abc;1").empty(), ());
  TEST(indoor::ParseLevels("2-0").empty(), ());
  TEST(indoor::ParseLevels("1e20").empty(), ());
  TEST(indoor::ParseLevels("0-100000").empty(), ());
}

UNIT_TEST(IndoorLevel_LevelsContain)
{
  TEST(indoor::LevelsContain("0;1", 1.0), ());
  TEST(!indoor::LevelsContain("0;1", 2.0), ());
  // Both sides are normalized, so a raw tag value matches its own snapped level.
  TEST(indoor::LevelsContain("1.3", 1.3), ());
  TEST(indoor::LevelsContain("1.3", 1.5), ());
  // An absent or unparsable value matches nothing, including the ground floor.
  TEST(!indoor::LevelsContain("", 0.0), ());
  TEST(!indoor::LevelsContain("G", 0.0), ());
}

UNIT_TEST(IndoorLevel_FormatLevel)
{
  TEST_EQUAL(indoor::FormatLevel(0.0), "0", ());
  TEST_EQUAL(indoor::FormatLevel(-1.0), "-1", ());
  TEST_EQUAL(indoor::FormatLevel(1.5), "1.5", ());
  TEST_EQUAL(indoor::FormatLevel(-1.5), "-1.5", ());
  // Rounded away, so the label always names a level ParseLevels can return.
  TEST_EQUAL(indoor::FormatLevel(0.25), "0.5", ());
}

UNIT_TEST(IndoorLevel_FormatRoundTrips)
{
  // FormatLevel is the canonical form, so every label must re-parse to exactly one level.
  for (double const level : {0.0, 1.0, -1.0, 1.5, -1.5, 9.0, -2.5})
  {
    auto const parsed = indoor::ParseLevels(indoor::FormatLevel(level));
    TEST_EQUAL(parsed, Levels({level}), (level));
  }
}
}  // namespace indoor_level_tests
