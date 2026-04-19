#include "duration.hpp"

#include "platform/locale.hpp"

#include "i18n/localisation_translation.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

namespace platform
{
namespace
{
using namespace std::chrono;

static constexpr std::string_view kNoSpace = "";

unsigned long SecondsToUnits(seconds duration, Duration::Units unit)
{
  switch (unit)
  {
  case Duration::Units::Days: return duration_cast<days>(duration).count();
  case Duration::Units::Hours: return duration_cast<hours>(duration).count();
  case Duration::Units::Minutes: return duration_cast<minutes>(duration).count();
  default: UNREACHABLE();
  }
}

seconds UnitsToSeconds(long value, Duration::Units unit)
{
  switch (unit)
  {
  case Duration::Units::Days: return days(value);
  case Duration::Units::Hours: return hours(value);
  case Duration::Units::Minutes: return minutes(value);
  default: UNREACHABLE();
  }
}

std::string_view GetUnitSeparator(Locale const & locale)
{
  static constexpr auto kEmptyNumberUnitSeparatorLocales =
      std::array{"en", "de", "fr", "he", "fa", "ja", "ko", "mr", "th", "tr", "vi", "zh"};
  bool const isEmptySeparator = base::IsExist(kEmptyNumberUnitSeparatorLocales, locale.m_language);
  return isEmptySeparator ? kNoSpace : kNarrowNonBreakingSpace;
}

std::string_view GetUnitsGroupingSeparator(Locale const & locale)
{
  static constexpr auto kEmptyGroupingSeparatorLocales = std::array{"ja", "zh"};
  bool const isEmptySeparator = base::IsExist(kEmptyGroupingSeparatorLocales, locale.m_language);
  return isEmptySeparator ? kNoSpace : kNonBreakingSpace;
}

std::string GetString(Duration const & duration, std::initializer_list<Duration::Units> units,
                      std::string_view unitSeparator, std::string_view groupingSeparator)
{
  ASSERT(units.size(), ());
  ASSERT(base::IsSortedAndUnique(units), ());

  if (SecondsToUnits(duration.GetUnderlying(), Duration::Units::Minutes) == 0)
    return std::to_string(0U).append(unitSeparator).append(GetUnitsString(Duration::Units::Minutes));

  std::string formattedTime;
  seconds remainingSeconds = duration.GetUnderlying();

  for (auto const unit : units)
  {
    unsigned long const unitsCount = SecondsToUnits(remainingSeconds, unit);
    if (unitsCount > 0)
    {
      if (!formattedTime.empty())
        formattedTime.append(groupingSeparator);
      formattedTime.append(std::to_string(unitsCount).append(unitSeparator).append(GetUnitsString(unit)));
      remainingSeconds -= UnitsToSeconds(unitsCount, unit);
    }
  }
  return formattedTime;
}
}  // namespace

std::string GetLocalizedString(Duration const & duration, std::initializer_list<Duration::Units> units,
                               Locale const & locale)
{
  return GetString(duration, std::move(units), GetUnitSeparator(locale), GetUnitsGroupingSeparator(locale));
}

std::string GetPlatformLocalizedString(Duration const & duration)
{
  struct InitSeparators
  {
    std::string_view m_unitSep, m_groupingSep;
    InitSeparators()
    {
      auto const loc = GetCurrentLocale();
      m_unitSep = GetUnitSeparator(loc);
      m_groupingSep = GetUnitsGroupingSeparator(loc);
    }
  };
  static InitSeparators seps;

  return GetString(duration, {Duration::Units::Days, Duration::Units::Hours, Duration::Units::Minutes}, seps.m_unitSep,
                   seps.m_groupingSep);
}

std::string GetUnitsString(Duration::Units unit)
{
  switch (unit)
  {
  case Duration::Units::Minutes: return localisation::TranslatedInterfaceText("minute");
  case Duration::Units::Hours: return localisation::TranslatedInterfaceText("hour");
  case Duration::Units::Days: return localisation::TranslatedInterfaceText("day");
  default: UNREACHABLE();
  }
}
}  // namespace platform
