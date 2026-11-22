#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace editor
{
// Days that share the same collection times.
struct CollectionTimesGroup
{
  bool operator==(CollectionTimesGroup const & rhs) const = default;

  // Same numbering as osmoh::Weekday and java.util.Calendar with Sunday as 1, listed from Monday.
  std::vector<int32_t> m_weekdays;
  // Minutes since midnight in ascending order.
  std::vector<int32_t> m_minutes;
};

using CollectionTimes = std::vector<CollectionTimesGroup>;

// Turns a collection_times value into a weekly schedule.
// Returns an empty schedule for values with dates, nth weekdays, time ranges or other syntax that does not fit a week.
CollectionTimes ParseCollectionTimes(std::string const & value);

// Formats one group per line like "Mo-Fr 17:30" using OSM weekday names.
std::string ToString(CollectionTimes const & times);
}  // namespace editor
