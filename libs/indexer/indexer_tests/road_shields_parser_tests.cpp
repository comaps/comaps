#include "testing/testing.hpp"

#include "indexer/road_shields_parser.hpp"

UNIT_TEST(RoadShields_DisplayRef)
{
  using ftypes::GetRoadShieldDisplayRef;

  TEST_EQUAL(GetRoadShieldDisplayRef(""), "", ());
  TEST_EQUAL(GetRoadShieldDisplayRef("SP246"), "SP246", ());
  TEST_EQUAL(GetRoadShieldDisplayRef("US 101 South"), "US 101 South", ());
  TEST_EQUAL(GetRoadShieldDisplayRef("IT:VI/SP246"), "SP246", ());
  TEST_EQUAL(GetRoadShieldDisplayRef("e-road/E 67;IT:VI/SP246;A 4"), "E 67;SP246;A 4", ());
  TEST_EQUAL(GetRoadShieldDisplayRef(";e-road/E 67;;A 4;"), "E 67;A 4", ());
  TEST_EQUAL(GetRoadShieldDisplayRef("network/;A 4"), "A 4", ());

  auto const shields = ftypes::GetRoadShields("Italy", "IT:VI/SP246", ftypes::HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_name, "SP246", ());
  TEST_EQUAL(shields[0].m_type, ftypes::RoadShieldType::Generic_Blue_Bordered, ());
}

UNIT_TEST(RoadShields_Brazil)
{
  using namespace ftypes;

  auto shields = GetRoadShields("Brazil", "BR-116", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_National, ());
  TEST_EQUAL(shields[0].m_name, "116", ());

  // Leading zeros are kept, like on the real signs.
  shields = GetRoadShields("Brazil", "BR-040", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_National, ());
  TEST_EQUAL(shields[0].m_name, "040", ());

  shields = GetRoadShields("Brazil", "RS-410", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_State, ());
  TEST_EQUAL(shields[0].m_name, "410", ());
  TEST_EQUAL(shields[0].m_additionalText, "RS", ());

  shields = GetRoadShields("Brazil", "BR-290;RS-122", HighwayClass::Primary);
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_National, ());
  TEST_EQUAL(shields[0].m_name, "290", ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Brazil_State, ());
  TEST_EQUAL(shields[1].m_additionalText, "RS", ());

  // Special state road classes: ERS/VRS/RSC (Rio Grande do Sul), MGC/LMG (Minas Gerais),
  // PRC (Paraná) display the plain state code on the shield.
  for (auto const & ref : {"ERS-123", "VRS-123", "RSC-123"})
  {
    shields = GetRoadShields("Brazil", ref, HighwayClass::Secondary);
    TEST_EQUAL(shields.size(), 1, (ref));
    TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_State, (ref));
    TEST_EQUAL(shields[0].m_name, "123", (ref));
    TEST_EQUAL(shields[0].m_additionalText, "RS", (ref));
  }

  shields = GetRoadShields("Brazil", "MGC-120;LMG-808", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_additionalText, "MG", ());
  TEST_EQUAL(shields[1].m_additionalText, "MG", ());

  shields = GetRoadShields("Brazil", "PRC-280", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_State, ());
  TEST_EQUAL(shields[0].m_additionalText, "PR", ());

  // Coincident state roads (same number as the overlapping federal highway) are hidden,
  // regardless of the order in the ref.
  for (auto const & ref : {"BR-453;RSC-453", "RSC-453;BR-453", "BR-453;MG-453"})
  {
    shields = GetRoadShields("Brazil", ref, HighwayClass::Trunk);
    TEST_EQUAL(shields.size(), 1, (ref));
    TEST_EQUAL(shields[0].m_type, RoadShieldType::Brazil_National, (ref));
    TEST_EQUAL(shields[0].m_name, "453", (ref));
  }

  // Different numbers are not coincident: both shields stay.
  shields = GetRoadShields("Brazil", "BR-453;RSC-470", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 2, ());

  // Unknown formats fall back to the default shield with the full ref.
  shields = GetRoadShields("Brazil", "SPA 262/310", HighwayClass::Secondary);
  for (auto const & shield : shields)
    TEST_EQUAL(shield.m_type, RoadShieldType::Default, ());
}

UNIT_TEST(RoadShields_Norway)
{
  using namespace ftypes;

  RoadShieldsSetT shields;
  for (auto const & ref : {"E6", "E 6"})
  {
    shields = GetRoadShields("Norway", ref, HighwayClass::Motorway);
    TEST_EQUAL(shields.size(), 1, (ref));
    TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, (ref));
    TEST_EQUAL(shields[0].m_name, ref, (ref));
  }

  // The E-road designation takes precedence over the underlying highway class.
  shields = GetRoadShields("Norway", "e-road/E 16", HighwayClass::Primary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Norway", "7", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Norway", "123", HighwayClass::Primary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_White_Bordered, ());

  shields = GetRoadShields("Norway", "Ring 3", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Pill_White_Bordered, ());
  TEST_EQUAL(shields[0].m_name, "Ring 3", ());

  shields = GetRoadShields("Norway", "Ring 2", HighwayClass::Primary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Pill_White_Bordered, ());

  // Four-digit county road numbers and municipal road numbers are not signed.
  TEST(GetRoadShields("Norway", "1234", HighwayClass::Secondary).empty(), ());
  shields = GetRoadShields("Norway", "Ring 4", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Pill_White_Bordered, ());
  TEST(GetRoadShields("Norway", "42", HighwayClass::LivingStreet).empty(), ());
}

UNIT_TEST(RoadShields_Romania)
{
  using namespace ftypes;

  auto shields = GetRoadShields("Romania", "A1", HighwayClass::Motorway);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green_Bordered, ());
  TEST_EQUAL(shields[0].m_name, "A1", ());

  shields = GetRoadShields("Romania", "DEx12", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red_Bordered, ());
  TEST_EQUAL(shields[0].m_name, "DEx12", ());

  // DN/DJ/DC symbols carry no lettering, so only the number is drawn.
  shields = GetRoadShields("Romania", "DN1", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "1", ());
  TEST_EQUAL(shields[0].GetShieldText(), "DN1", ());

  shields = GetRoadShields("Romania", "DJ105", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_County, ());
  TEST_EQUAL(shields[0].m_name, "105", ());
  TEST_EQUAL(shields[0].GetShieldText(), "DJ105", ());

  shields = GetRoadShields("Romania", "DC7", HighwayClass::Tertiary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_Local, ());
  TEST_EQUAL(shields[0].m_name, "7", ());
  TEST_EQUAL(shields[0].GetShieldText(), "DC7", ());

  shields = GetRoadShields("Romania", "DN7C", HighwayClass::Primary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "7C", ());

  shields = GetRoadShields("Romania", "DJ105A", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_County, ());
  TEST_EQUAL(shields[0].m_name, "105A", ());

  // Ring roads are numbered "C" + the city's initial instead of a number. The
  // only 2 examples so far are the DNCB for the Bucharest ring road and DNCT
  // for the Timisoara ring road.
  shields = GetRoadShields("Romania", "DNCB", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "CB", ());
  TEST_EQUAL(shields[0].GetShieldText(), "DNCB", ());

  shields = GetRoadShields("Romania", "DNCT", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "CT", ());

  // "DEx" must be displayed over the other "D"-prefixed classes.
  shields = GetRoadShields("Romania", "DEx4", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red_Bordered, ());

  // A national road that also carries a county number is a single road: only the national
  // shield is drawn, no matter which of the two refs comes first in the tag.
  shields = GetRoadShields("Romania", "DN22;DJ221B", HighwayClass::Primary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "22", ());

  shields = GetRoadShields("Romania", "DJ191C;DN1T", HighwayClass::Secondary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "1T", ());

  // A county road co-signed with a communal one keeps only the county shield.
  shields = GetRoadShields("Romania", "DJ101;DC5", HighwayClass::Tertiary);
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_County, ());
  TEST_EQUAL(shields[0].m_name, "101", ());

  // Two refs of the same class are a genuine multiplex and are both displayed.
  shields = GetRoadShields("Romania", "DN1;DN7", HighwayClass::Trunk);
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[0].m_name, "1", ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Romania_National, ());
  TEST_EQUAL(shields[1].m_name, "7", ());
}

UNIT_TEST(RoadShields_Smoke)
{
  using namespace ftypes;

  // TODO: Fix broken tests to make code compile
  /*
  auto shields = GetRoadShields("France", "D 116A");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Orange, ());

  shields = GetRoadShields("Belarus", "M1");  // latin letter M
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Red, ());

  shields = GetRoadShields("Belarus", "Е2");  // cyrillic letter Е
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Ukraine", "Р50");  // cyrillic letter Р
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Malaysia", "AH7");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Germany", "A 3;A 7");
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Generic_Blue, ());

  shields = GetRoadShields("Germany", "blue/A 31;national/B 2R");
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Blue, ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Generic_Orange, ());

  shields = GetRoadShields("Germany", "TMC 33388 (St 2047)");
  TEST_EQUAL(shields.size(), 0, ());

  shields = GetRoadShields("US", "US:IN");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Default, ());

  shields = GetRoadShields("US", "SR 38;US:IN");
  TEST_EQUAL(shields.size(), 2, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_White, ());
  TEST_EQUAL(shields[1].m_type, RoadShieldType::Default, ());

  shields = GetRoadShields("Switzerland", "e-road/E 67");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Green, ());

  shields = GetRoadShields("Estonia", "ee:national/27;ee:local/7841171");
  TEST_EQUAL(shields.size(), 1, ());
  TEST_EQUAL(shields[0].m_type, RoadShieldType::Generic_Orange, ());
  */
}
