#include "testing/testing.hpp"

#include "generator/address_enricher.hpp"
#include "generator/addresses_collector.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_writer.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/point2d.hpp"

using generator::tests_support::TestWithClassificator;
using platform::tests_support::ScopedFile;

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

UNIT_CLASS_TEST(TestWithClassificator, AddressEnricher_TempAddrFormat_Valid)
{
  using namespace generator;

  ScopedFile sf("test_format_header.tempaddr", ScopedFile::Mode::DoNotCreate);
  {
    FileWriter w(sf.GetFullPath());
    uint8_t const header[2] = {AddressEnricher::kTempAddrMagic, AddressEnricher::kTempAddrVersion};
    w.Write(header, 2);

    AddressEnricher::RawEntryBase e;
    // Use a purely non-numeric HN so GetHNRange() returns kInvalidRange
    // and the alphanumeric path fires without calling Match() (which
    // requires street data that doesn't exist in this unit test).
    e.m_from = "A";
    e.m_to = "A";
    e.m_street = "Test St";
    e.m_postcode = "V1A 1A1";
    e.m_interpol = feature::InterpolType::None;
    e.m_editable = true;
    e.Save(w);

    std::vector<int64_t> const pts = {PointToInt64Obsolete({0.0, 0.0}, kPointCoordBits)};
    rw::Write(w, pts);
  }

  AddressEnricher enricher;
  int count = 0;
  enricher.ProcessRawEntries(sf.GetFullPath(), [&count](feature::FeatureBuilder const &) { ++count; });
  TEST_EQUAL(count, 1, ());
}
