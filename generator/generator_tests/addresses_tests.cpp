#include "testing/testing.hpp"

#include "generator/addresses_collector.hpp"

UNIT_TEST(GenerateAddresses_AddressInfo_FormatRange)
{
  generator::AddressesHolder::AddressInfo info;

  info.m_house = "3000";
  info.m_house2 = "3100";
  TEST_EQUAL(info.FormatRange(), "3000:3100", ());

  info.m_house = "72B";
  info.m_house2 = "14 стр 1";
  TEST_EQUAL(info.FormatRange(), "14:72", ());

  info.m_house = "foo";
  info.m_house2 = "bar";
  TEST_EQUAL(info.FormatRange(), "", ());
}

UNIT_TEST(AddressEnricher_DiscretePoint_HouseNumberLabel)
{
  auto makeLabel = [](std::string const & from, std::string const & to) -> std::string
  { return (from == to) ? from : from + " - " + to; };

  TEST_EQUAL(makeLabel("42", "42"), "42", ());
  TEST_EQUAL(makeLabel("100", "100"), "100", ());
  TEST_EQUAL(makeLabel("100", "198"), "100 - 198", ());
  TEST_EQUAL(makeLabel("1", "99"), "1 - 99", ());
}
