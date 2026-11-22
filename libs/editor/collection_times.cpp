#include "editor/collection_times.hpp"

#include "3party/opening_hours/opening_hours.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <optional>
#include <sstream>

namespace editor
{
namespace
{
using DayTimes = std::optional<std::vector<int32_t>>;

// Monday first, values are osmoh::Weekday.
std::array<int32_t, 7> constexpr kWeekOrder = {2, 3, 4, 5, 6, 7, 1};

bool ApplyDays(osmoh::RuleSequence const & rule, std::array<bool, 8> & days)
{
  days.fill(false);
  auto const & weekdays = rule.GetWeekdays();
  if (weekdays.IsEmpty())
  {
    days.fill(true);
    return true;
  }

  for (auto const & range : weekdays.GetWeekdayRanges())
  {
    if (range.HasNth() || range.HasOffset())
      return false;
    for (auto const wd : kWeekOrder)
      if (range.HasWday(static_cast<osmoh::Weekday>(wd)))
        days[wd] = true;
  }
  // A rule like "PH off" has no weekdays to show in a weekly schedule.
  return true;
}

bool GetMinutes(osmoh::RuleSequence const & rule, std::vector<int32_t> & minutes)
{
  minutes.clear();
  for (auto const & span : rule.GetTimes())
  {
    if (!span.IsPointInTime() || !span.GetStart().IsHoursMinutes())
      return false;
    minutes.push_back(static_cast<int32_t>(span.GetStart().GetHourMinutes().GetDurationCount()));
  }
  return !minutes.empty();
}
}  // namespace

CollectionTimes ParseCollectionTimes(std::string const & value)
{
  osmoh::OpeningHours const oh(value);
  if (!oh.IsValid() || oh.HasYearSelector() || oh.HasMonthSelector() || oh.HasWeekSelector())
    return {};

  std::array<DayTimes, 8> week;
  bool additional = false;
  for (auto const & rule : oh.GetRule())
  {
    using Modifier = osmoh::RuleSequence::Modifier;
    auto const modifier = rule.GetModifier();
    if (modifier == Modifier::Unknown || modifier == Modifier::Comment || rule.HasComment())
      return {};

    std::array<bool, 8> days;
    if (!ApplyDays(rule, days))
      return {};

    std::vector<int32_t> minutes;
    if (modifier != Modifier::Closed && !GetMinutes(rule, minutes))
      return {};

    for (auto const wd : kWeekOrder)
    {
      if (!days[wd])
        continue;
      auto & dayTimes = week[wd];
      if (modifier == Modifier::Closed)
        dayTimes = std::vector<int32_t>{};
      else if (additional && dayTimes)
        dayTimes->insert(dayTimes->end(), minutes.begin(), minutes.end());
      else
        dayTimes = minutes;
    }

    auto const & separator = rule.GetAnySeparator();
    if (separator == "||")
      return {};
    additional = separator == ",";
  }

  CollectionTimes result;
  for (auto const wd : kWeekOrder)
  {
    auto & dayTimes = week[wd];
    if (!dayTimes || dayTimes->empty())
      continue;

    std::sort(dayTimes->begin(), dayTimes->end());
    dayTimes->erase(std::unique(dayTimes->begin(), dayTimes->end()), dayTimes->end());

    auto it = std::find_if(result.begin(), result.end(),
                           [&](CollectionTimesGroup const & g) { return g.m_minutes == *dayTimes; });
    if (it == result.end())
      result.push_back({{wd}, *dayTimes});
    else
      it->m_weekdays.push_back(wd);
  }
  return result;
}

std::string ToString(CollectionTimes const & times)
{
  static char const * const kDays[] = {"", "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
  auto const order = [](int32_t wd) { return (wd + 5) % 7; };

  std::ostringstream ss;
  for (size_t g = 0; g < times.size(); ++g)
  {
    if (g > 0)
      ss << '\n';

    auto const & days = times[g].m_weekdays;
    for (size_t i = 0; i < days.size();)
    {
      size_t j = i;
      while (j + 1 < days.size() && order(days[j + 1]) == order(days[j]) + 1)
        ++j;
      if (i > 0)
        ss << ',';
      ss << kDays[days[i]];
      if (j > i)
        ss << (j == i + 1 ? "," : "-") << kDays[days[j]];
      i = j + 1;
    }

    char const * sep = " ";
    for (auto const m : times[g].m_minutes)
    {
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", m / 60, m % 60);
      ss << sep << buf;
      sep = ", ";
    }
  }
  return ss.str();
}
}  // namespace editor
