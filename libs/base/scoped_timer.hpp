#pragma once

#include "base/logging.hpp"
#include "base/timer.hpp"

#include <string>

namespace base
{
/**
 * @brief Scoped timer with log output.
 *
 * The timer starts counting from the moment it is constructed. When it goes out of scope and its
 * destructor is triggered, it logs the time that has elapsed.
 *
 * Usage example:
 *
 * @code{.cpp}
 * {
 *   ScopedTimerWithLog timer("Sample timer", Measure::MilliSeconds, LogLevel::LINFO);
 *
 *   // do stuff
 *   DoStuff();
 *
 *   // when reaching this point (end of scope), the elapsed time is logged, for example:
 *   // I(2) 17.38334 base/timer.cpp:185 ~ScopedTimerWithLog(): Sample timer time: 6 ms
 * }
 * @endcode
 */
class ScopedTimerWithLog
{
public:
  enum class Measure
  {
    MilliSeconds,
    Seconds,
  };

  /**
   * @brief Constructs a new instance.
   *
   * @param timerName A name or description for the timer, which will be used for log output.
   * @param measure The unit of time to use for log output.
   * @param level The log level to use for log output.
   */
  explicit ScopedTimerWithLog(std::string const & timerName, Measure measure = Measure::MilliSeconds,
                              LogLevel level = LogLevel::LINFO);
  ~ScopedTimerWithLog();

private:
  /** @brief The name or description for the timer, which will be used for log output. */
  std::string m_name;

  /** @brief The unit of time to use for log output. */
  Measure m_measure;

  /** @brief The log level for timer output. */
  LogLevel m_level;

  /** @brief The internal timer. */
  HighResTimer m_timer;
};
}  // namespace base
