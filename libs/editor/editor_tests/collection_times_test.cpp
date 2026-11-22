#include "testing/testing.hpp"

#include "editor/collection_times.hpp"
#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

#include "3party/opening_hours/opening_hours.hpp"

using namespace editor;

namespace
{
int32_t constexpr kSu = 1, kMo = 2, kTu = 3, kWe = 4, kTh = 5, kFr = 6, kSa = 7;
}  // namespace

UNIT_TEST(CollectionTimes_Simple)
{
  auto const ct = ParseCollectionTimes("Mo-Fr 17:30; Sa 12:00");
  TEST_EQUAL(ct.size(), 2, ());
  TEST_EQUAL(ct[0].m_weekdays, (std::vector<int32_t>{kMo, kTu, kWe, kTh, kFr}), ());
  TEST_EQUAL(ct[0].m_minutes, (std::vector<int32_t>{17 * 60 + 30}), ());
  TEST_EQUAL(ct[1].m_weekdays, (std::vector<int32_t>{kSa}), ());
  TEST_EQUAL(ct[1].m_minutes, (std::vector<int32_t>{12 * 60}), ());
  TEST_EQUAL(ToString(ct), "Mo-Fr 17:30\nSa 12:00", ());
}

UNIT_TEST(CollectionTimes_GroupsAndOverrides)
{
  // Later rules replace earlier ones for the days they name.
  auto ct = ParseCollectionTimes("Mo-Sa 09:00,17:00; We 12:00; Su off");
  TEST_EQUAL(ToString(ct), "Mo,Tu,Th-Sa 09:00, 17:00\nWe 12:00", ());

  ct = ParseCollectionTimes("17:00, Sa 10:00");
  TEST_EQUAL(ToString(ct), "Mo-Fr,Su 17:00\nSa 10:00, 17:00", ());

  // Common in UK data, the comma joins two rules.
  ct = ParseCollectionTimes("Mo-Fr 17:30, Sa 10:30");
  TEST_EQUAL(ToString(ct), "Mo-Fr 17:30\nSa 10:30", ());

  ct = ParseCollectionTimes("Mo-Fr 16:15; Sa 11:00; PH off");
  TEST_EQUAL(ToString(ct), "Mo-Fr 16:15\nSa 11:00", ());

  ct = ParseCollectionTimes("Mo,We,Fr 08:00");
  TEST_EQUAL(ct.size(), 1, ());
  TEST_EQUAL(ct[0].m_weekdays, (std::vector<int32_t>{kMo, kWe, kFr}), ());
  TEST_EQUAL(ToString(ct), "Mo,We,Fr 08:00", ());

  ct = ParseCollectionTimes("Su 10:00; Sa 10:00");
  TEST_EQUAL(ct[0].m_weekdays, (std::vector<int32_t>{kSa, kSu}), ());
  TEST_EQUAL(ToString(ct), "Sa,Su 10:00", ());
}

UNIT_TEST(CollectionTimes_Unsupported)
{
  TEST(ParseCollectionTimes("").empty(), ());
  TEST(ParseCollectionTimes("garbage").empty(), ());
  TEST(ParseCollectionTimes("Mo-Fr 16:00-18:00").empty(), ());
  TEST(ParseCollectionTimes("Jan-Mar Mo 10:00").empty(), ());
  TEST(ParseCollectionTimes("Su[1] 10:00").empty(), ());
  TEST(ParseCollectionTimes("Mo-Fr 17:00 \"varies\"").empty(), ());
  TEST(ParseCollectionTimes("off").empty(), ());
}

UNIT_TEST(CollectionTimes_OpeningHoursUnaffected)
{
  // A point in time is valid syntax but says nothing about a place being open.
  osmoh::OpeningHours const oh("Mo-Su 17:00");
  TEST(oh.IsValid(), ());
  TEST(oh.IsUnknown(time(nullptr)), ());

  ui::TimeTableSet tts;
  TEST(!MakeTimeTableSet(oh, tts), ());
}
