#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"

namespace highway_full_name_tests
{

UNIT_TEST(HighwayFullNameTest_NotHighway)
{
  OsmElement notRoad =
      MakeOsmElement(0 /* id */, {{"name", "300 East"}, {"name:prefix", "North"}, {"name:full", "North 300 East"}},
                     OsmElement::EntityType::Way);

  TEST_EQUAL(notRoad.GetTag("name"), "300 East", ());
  TEST_EQUAL(notRoad.GetTag("name:prefix"), "North", ());
  TEST_EQUAL(notRoad.GetTag("name:full"), "North 300 East", ());
}

UNIT_TEST(HighwayFullNameTest_ReplaceName)
{
  OsmElement road1 = MakeOsmElement(
      1 /* id */,
      {{"highway", "residential"}, {"name", "300 East"}, {"name:prefix", "North"}, {"name:full", "North 300 East"}},
      OsmElement::EntityType::Way);

  TEST_EQUAL(road1.GetTag("name"), "North 300 East", ());
}

UNIT_TEST(HighwayFullNameTest_MissingName)
{
  OsmElement road2 = MakeOsmElement(
      2 /* id */, {{"highway", "residential"}, {"name:prefix", "North"}, {"name:full", "North 300 East"}},
      OsmElement::EntityType::Way);

  TEST(!road2.HasTag("name"), ());
}

UNIT_TEST(HighwayFullNameTest_MissingNamePrefix)
{
  OsmElement road3 =
      MakeOsmElement(3 /* id */, {{"highway", "residential"}, {"name", "300 East"}, {"name:full", "North 300 East"}},
                     OsmElement::EntityType::Way);

  TEST_EQUAL(road3.GetTag("name"), "300 East", ());
  TEST(!road3.HasTag("name:prefix"), ());
}

UNIT_TEST(HighwayFullNameTest_MissingNameFull)
{
  OsmElement road4 =
      MakeOsmElement(4 /* id */, {{"highway", "residential"}, {"name", "300 East"}, {"name:prefix", "North"}},
                     OsmElement::EntityType::Way);

  TEST_EQUAL(road4.GetTag("name"), "300 East", ());
  TEST(!road4.HasTag("name:full"), ());
}

UNIT_TEST(HighwayFullNameTest_OnlyName)
{
  OsmElement road5 =
      MakeOsmElement(5 /* id */, {{"highway", "residential"}, {"name", "300 East"}}, OsmElement::EntityType::Way);

  TEST_EQUAL(road5.GetTag("name"), "300 East", ());
  TEST(!road5.HasTag("name:prefix"), ());
  TEST(!road5.HasTag("name:full"), ());
}

UNIT_TEST(HighwayFullNameTest_OnlyFullName)
{
  OsmElement road6 = MakeOsmElement(6 /* id */, {{"highway", "residential"}, {"name:full", "North 300 East"}},
                                    OsmElement::EntityType::Way);

  TEST_EQUAL(road6.GetTag("name:full"), "North 300 East", ());
  TEST(!road6.HasTag("name"), ());
  TEST(!road6.HasTag("name:prefix"), ());
}

UNIT_TEST(HighwayFullNameTest_NotMatching)
{
  OsmElement road7 = MakeOsmElement(
      7 /* id */,
      {{"highway", "residential"}, {"name", "300 East"}, {"name:prefix", "South"}, {"name:full", "North 300 East"}},
      OsmElement::EntityType::Way);

  TEST_EQUAL(road7.GetTag("name"), "300 East", ());
}
}  // namespace highway_full_name_tests
