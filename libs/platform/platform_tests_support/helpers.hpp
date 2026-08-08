#pragma once

#include "3party/opening_hours/opening_hours.hpp"

#include <cstdint>

namespace platform::tests_support
{
using Month = osmoh::MonthDay::Month;
using Weekday = osmoh::Weekday;

time_t GetUnixtimeByDate(uint16_t year, Month month, uint8_t monthDay, uint8_t hours, uint8_t minutes);
time_t GetUnixtimeByWeekday(uint16_t year, Month month, Weekday weekday, uint8_t hours, uint8_t minutes);
}  // namespace platform::tests_support
