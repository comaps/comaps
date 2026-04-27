#include "testing/testing.hpp"

#include "generator/address_enricher.hpp"
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

UNIT_TEST(AddressEnricher_GetHNRange_Alphanumeric)
{
  using RawEntry = generator::AddressEnricher::RawEntryBase;
  RawEntry e;

  e.m_from = "100"; e.m_to = "198";
  TEST_NOT_EQUAL(e.GetHNRange(), RawEntry::kInvalidRange, ());

  e.m_from = "1"; e.m_to = "1";
  TEST_NOT_EQUAL(e.GetHNRange(), RawEntry::kInvalidRange, ());

  e.m_from = "123A"; e.m_to = "123A";
  TEST_NOT_EQUAL(e.GetHNRange(), RawEntry::kInvalidRange, ());
  TEST_EQUAL(e.GetHNRange(), std::make_pair(uint64_t(123), uint64_t(123)), ());

  e.m_from = "foo"; e.m_to = "bar";
  TEST_EQUAL(e.GetHNRange(), RawEntry::kInvalidRange, ());
}
