#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace traffic
{
  /**
   * @brief Date and time decoded from ISO 8601.
   *
   * `IsoTime` is an opaque type. It is only guaranteed to be capable of holding a timestamp
   * converted from ISO 8601 which refers to the same UTC time as its ISO 8601 representation.
   * Time zone information is not guaranteed to be preserved: `13:37+01:00` may be returned e.g. as
   * `12:37Z` or `06:37-06:00`.
   */
  class IsoTime
  {
  public:
    /**
     * @brief Parses time in ISO 8601 format from a string and stores it in an `IsoTime`.
     *
     * ISO 8601 timestamps have the format `yyyy-mm-ddThh:mm:ss[.sss]`, optionally followed by a UTC
     * offset. For example, `2019-11-01T11:45:42+01:00` refers to 11:45:42 in the UTC+1 timezone, which
     * is 10:45:42 UTC.
     *
     * A UTC offset of `Z` denotes UTC and is equivalent to `+00:00` or `-00:00`. UTC is also assumed
     * if no UTC offset is specified.
     *
     * The UTC offset can be specified as `hh:mm`, `hhmm` or `hh`.
     *
     * Seconds can be specified as integer or float, but will be rounded to the nearest integer. For
     * example, 42.645 seconds will be rounded to 43 seconds.
     *
     * @param timeString Time in ISO8601 format
     * @return An `IsoTime` instance corresponding to `timeString`, or `std::nullopt` if `timeString` is not a valid ISO8601 time string.
     */
    static std::optional<IsoTime> ParseIsoTime(std::string const & timeString);

    /**
     * @brief Returns an `IsoTime` corresponding to current wall clock time.
     *
     * @return An `IsoTime` corresponding to current wall clock time.
     */
    static IsoTime Now();

    /**
     * @brief Returns an `IsoTime` which is a specified number of seconds from current wall clock time.
     *
     * @param offset The offset in seconds, positive for a time in the future
     * @return An `IsoTime` that is `offset` seconds from current wall clock time.
     */
    static IsoTime SecondsFromNow(double offset);

    /**
     * @brief Whether the instance refers to a point of time in the past.
     *
     * Comparison is against system time.
     *
     * @return true if in the past, false of not.
     */
    bool IsPast() const;

    /**
     * @brief Shifts time to the present.
     *
     * This method is intended for testing. It shifts the timestamp by a fixed amount, so that
     * `nowRef` corresponds to current time. After this method returns, the timestamp will have the
     * same offset from current time that it had from `nowRef` at the time the call was made.
     *
     * @param nowRef
     */
    void Shift(IsoTime const & nowRef);

    /**
     * @brief Returns a string representation of the instance.
     * @return The timestamp in ISO 8601 format.
     */
    std::string ToString() const;

    bool operator== (const IsoTime & rhs) const;
    bool operator< (const IsoTime & rhs) const;
    bool operator<= (const IsoTime & rhs) const;
    bool operator> (const IsoTime & rhs) const;
    bool operator>= (const IsoTime & rhs) const;
  private:
    friend std::string DebugPrint(IsoTime const & time);

    IsoTime(std::chrono::time_point<std::chrono::system_clock> tp);

    std::chrono::time_point<std::chrono::system_clock> m_tp;
  };

std::string DebugPrint(IsoTime const & time);
}  // namespace traffic
