#include "traffic/iso_time.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <string>

#include "defines.hpp"
#include "private.h"

#include <boost/regex.hpp>

namespace traffic
{
  std::optional<IsoTime> IsoTime::ParseIsoTime(std::string timeString)
  {
    /*
     * TODO this is ugly because we need to work around some compiler deficiencies.
     *
     * Ideally, we would be using `std::chrono::time_point<std::chrono::utc_clock>` and parse the
     * string using `std::chrono::from_stream`, using `%FT%T%z` for the format string.
     * This works in GCC 14+ and is pleasantly liberal about the time zone format (all of +01, +0100
     * and +01:00 are parsed correctly). Alas, Ubuntu 24.04 (currently the default dev platform) comes
     * with GCC 13.2, which lacks this support. Clang, the only supported compiler for Android (and,
     * presumably, iOS), as of mid-2025, doesn’t support it at all.
     *
     * The workaround is therefore to use `std::chrono::time_point<std::chrono::system_clock>`, which
     * exposes the same API as its `utc_clock` counterpart, making transition at a later point easy.
     * In addition, however, it can be constructed from `std::time_t`, which we can generate from
     * `std::tm`. Unlike the other C legacy functions, gmtime is thread-safe.
     * Still not the prettiest way (as it relies on legacy C functions which are not
     * thread-safe), but the best we can get until we have proper compiler support for `from_stream`.
     *
     * Should we have support for `std::chrono:clock_cast` but not `std::chrono::from_stream`, we
     * could build a `std::chrono::sys_seconds` from the constutuent values and use
     * `std::chrono::clock_cast` to convert it to a `std::chrono::time_point`, based on whatever clock
     * is supported. This works on Linux (using `utc_clock`) as of mid-2025, but not on the primary
     * target platforms (Android and iOS) and has therefore been left out for uniformity (and
     * reproducibility of bugs).
     */
    /*
     * Regex for ISO 8601 time, with some tolerance for time zone offset. If matched, the matcher
     * will contain the following items:
     *
     *  0: 2019-11-01T11:55:42+01:00  (entire expression)
     *  1: 2019                       (year)
     *  2: 11                         (month)
     *  3: 01                         (day)
     *  4: 11                         (hour, local)
     *  5: 55                         (minute, local)
     *  6: 42.445                     (second, local, float)
     *  7: .445                       (fractional seconds)
     *  8: +01:00                     (complete UTC offset, or Z; blank if not specified)
     *  9: +01:00                     (complete UTC offset, blank for Z or of not specified)
     * 10: +01                        (UTC offset, hours with sign; blank for Z or if not specified)
     * 11: :00                        (UTC offset, minutes, prefixed with separator)
     * 12: 00                         (UTC offset, minutes, unsigned; blank for Z or if not specified)
     */
    static boost::regex iso8601Regex("([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):([0-9]{2}(\\.[0-9]*)?)(Z|(([+-][0-9]{2})(:?([0-9]{2}))?))?");

    boost::smatch iso8601Matcher;
    if (boost::regex_search(timeString, iso8601Matcher, iso8601Regex))
    {
      int offset_h = iso8601Matcher[10].matched ? std::stoi(iso8601Matcher[10]) : 0;
      int offset_m = iso8601Matcher[12].matched ? std::stoi(iso8601Matcher[12]) : 0;
      if (offset_h < 0)
        offset_m *= -1;

      std::tm tm = {};
      tm.tm_year = std::stoi(iso8601Matcher[1]) - 1900;
      tm.tm_mon = std::stoi(iso8601Matcher[2]) - 1;
      tm.tm_mday = std::stoi(iso8601Matcher[3]);
      tm.tm_hour = std::stoi(iso8601Matcher[4]) - offset_h;
      tm.tm_min = std::stoi(iso8601Matcher[5]) - offset_m;
      tm.tm_sec = std::stof(iso8601Matcher[6]) + 0.5f;

      std::time_t tt = timegm(&tm);

      std::chrono::time_point<std::chrono::system_clock> tp = std::chrono::system_clock::from_time_t(tt);

      IsoTime result(tp);
      return result;
    }
    else
    {
      LOG(LINFO, ("Not a valid ISO 8601 timestamp:", timeString));
      return std::nullopt;
    }
  }

  IsoTime IsoTime::Now()
  {
    return IsoTime(std::chrono::system_clock::now());
  }

  IsoTime::IsoTime(std::chrono::time_point<std::chrono::system_clock> tp)
    : m_tp(tp)
  {}

  bool IsoTime::IsPast()
  {
    return m_tp < std::chrono::system_clock::now();
  }\

  void IsoTime::Shift(IsoTime nowRef)
  {
    auto const offset = std::chrono::system_clock::now() - nowRef.m_tp;
    m_tp += offset;
  }

  std::string IsoTime::ToString() const
  {
    auto const tp_seconds = time_point_cast<std::chrono::seconds>(m_tp);
    auto const time_t = std::chrono::system_clock::to_time_t(tp_seconds);
    std::tm tm = *std::gmtime(&time_t);

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    ss << "Z";
    return ss.str();
  }

  bool IsoTime::operator< (const IsoTime & rhs) const
  {
    return m_tp < rhs.m_tp;
  }

  bool IsoTime::operator> (const IsoTime & rhs) const
  {
    return m_tp > rhs.m_tp;
  }

  std::string DebugPrint(IsoTime time)
  {
    std::ostringstream os;
    //os << std::put_time(&time.m_tm, "%Y-%m-%d %H:%M:%S %z");
    // %FT%T%z
    auto const time_t = std::chrono::system_clock::to_time_t(time.m_tp);
    std::tm tm = *std::gmtime(&time_t);
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S UTC");
    return os.str();
  }
}  // namespace traffic
