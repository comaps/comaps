#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"

#include <vector>

namespace indoor_level_tests
{
UNIT_TEST(IndoorLevel_ParseSingle)
{
  TEST_EQUAL(indoor::ParseLevels("0"), std::vector<double>({0.0}), ());
  TEST_EQUAL(indoor::ParseLevels("1"), std::vector<double>({1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("-1"), std::vector<double>({-1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("1.5"), std::vector<double>({1.5}), ());
  TEST_EQUAL(indoor::ParseLevels("-0.5"), std::vector<double>({-0.5}), ());
}

UNIT_TEST(IndoorLevel_ParseLists)
{
  TEST_EQUAL(indoor::ParseLevels("0;1;2"), std::vector<double>({0.0, 1.0, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("2;0;1"), std::vector<double>({0.0, 1.0, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("0,1"), std::vector<double>({0.0, 1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("-1;0"), std::vector<double>({-1.0, 0.0}), ());
  TEST_EQUAL(indoor::ParseLevels("0; 1"), std::vector<double>({0.0, 1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("0;0;1"), std::vector<double>({0.0, 1.0}), ());
}

UNIT_TEST(IndoorLevel_ParseRanges)
{
  TEST_EQUAL(indoor::ParseLevels("0-2"), std::vector<double>({0.0, 1.0, 2.0}), ());
  TEST_EQUAL(indoor::ParseLevels("-2--1"), std::vector<double>({-2.0, -1.0}), ());
  TEST_EQUAL(indoor::ParseLevels("-1-1"), std::vector<double>({-1.0, 0.0, 1.0}), ());
}

UNIT_TEST(IndoorLevel_ParseInvalid)
{
  TEST(indoor::ParseLevels("").empty(), ());
  TEST(indoor::ParseLevels("G").empty(), ());
  TEST(indoor::ParseLevels("ground").empty(), ());
  TEST(indoor::ParseLevels("abc;1").empty(), ());
  TEST(indoor::ParseLevels("2-0").empty(), ());  // inverted range
}

UNIT_TEST(IndoorLevel_LevelsContain)
{
  TEST(indoor::LevelsContain("0;1", 0.0), ());
  TEST(indoor::LevelsContain("0;1", 1.0), ());
  TEST(!indoor::LevelsContain("0;1", 2.0), ());
  TEST(indoor::LevelsContain("1.5", 1.5), ());
  TEST(!indoor::LevelsContain("1.5", 1.0), ());

  // Missing or unparsable values are treated as level 0.
  TEST(indoor::LevelsContain("", 0.0), ());
  TEST(!indoor::LevelsContain("", 1.0), ());
  TEST(indoor::LevelsContain("G", 0.0), ());
  TEST(!indoor::LevelsContain("G", -1.0), ());
}

UNIT_TEST(IndoorLevel_FormatLevel)
{
  TEST_EQUAL(indoor::FormatLevel(0.0), "0", ());
  TEST_EQUAL(indoor::FormatLevel(0.2), "0.2", ());
  TEST_EQUAL(indoor::FormatLevel(0.25), "0.25", ());
  TEST_EQUAL(indoor::FormatLevel(1.0), "1", ());
  TEST_EQUAL(indoor::FormatLevel(1.2), "1.2", ());
  TEST_EQUAL(indoor::FormatLevel(1.25), "1.25", ());
  TEST_EQUAL(indoor::FormatLevel(1.5), "1.5", ());
  TEST_EQUAL(indoor::FormatLevel(-0.2), "-0.2", ());
  TEST_EQUAL(indoor::FormatLevel(-0.25), "-0.25", ());
  TEST_EQUAL(indoor::FormatLevel(-0.5), "-0.5", ());
  TEST_EQUAL(indoor::FormatLevel(-1.0), "-1", ());
  TEST_EQUAL(indoor::FormatLevel(-1.2), "-1.2", ());
  TEST_EQUAL(indoor::FormatLevel(-1.25), "-1.25", ());
}

UNIT_TEST(IndoorLevel_FormatLevelLocale)
{
  double d1 = 0.25; // fractional test
  double d2 = 1.00; // whole test

  struct TestData
  {
    std::string localeName;
    std::string d1String;
    std::string d2String;
  };

  TestData testData[] = {// Locale, Fractional, Whole
                         {"en_US.UTF-8", "0.25", "1"},
                         {"es_ES.UTF-8", "0,25", "1"},
                         {"fr_FR.UTF-8", "0,25", "1"},
                         {"ru_RU.UTF-8", "0,25", "1"}};

  for (TestData const & data : testData)
  {
    Locale loc;

    if (!GetLocale(data.localeName, loc))
    {
      std::cout << "Locale '" << data.localeName << "' not found!! Skipping test..." << std::endl;
      continue;
    }

    TEST_EQUAL(indoor::FormatLevel(d1), data.d1String, ());
    TEST_EQUAL(indoor::FormatLevel(d2), data.d2String, ());
  }
}

UNIT_TEST(IndoorLevel_IsIndoorChecker)
{
  classificator::Load();
  Classificator const & c = classif();
  auto const & checker = ftypes::IsIndoorChecker::Instance();

  TEST(checker(c.GetTypeByPath({"indoor", "room"})), ());
  TEST(checker(c.GetTypeByPath({"indoor", "corridor"})), ());
  TEST(checker(c.GetTypeByPath({"indoor", "area"})), ());
  TEST(checker(c.GetTypeByPath({"indoor", "wall"})), ());
  TEST(checker(c.GetTypeByPath({"indoor", "door"})), ());

  TEST(!checker(c.GetTypeByPath({"building"})), ());
  TEST(!checker(c.GetTypeByPath({"entrance"})), ());
}
}  // namespace indoor_level_tests
