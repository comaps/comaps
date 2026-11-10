#include "base/scoped_timer.hpp"

#include "base/assert.hpp"

namespace base
{
ScopedTimerWithLog::ScopedTimerWithLog(std::string const & timerName, Measure measure, LogLevel level)
  : m_name(timerName)
  , m_measure(measure)
  , m_level(level)
{}

ScopedTimerWithLog::~ScopedTimerWithLog()
{
  switch (m_measure)
  {
  case Measure::MilliSeconds:
  {
    LOG(m_level, (m_name, "time:", m_timer.ElapsedMilliseconds(), "ms"));
    return;
  }
  case Measure::Seconds:
  {
    LOG(m_level, (m_name, "time:", m_timer.ElapsedSeconds(), "s"));
    return;
  }
  }
  UNREACHABLE();
}
}  // namespace base
