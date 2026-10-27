#pragma once

#include "indexer/road_shields_parser.hpp"

#include <string>

namespace routing
{
struct RoadShieldInfo
{
  /// An array of road shields for the target street
  ftypes::RoadShieldsSetT m_targetRoadShields;
  /// An array of road shields for the junction
  ftypes::RoadShieldsSetT m_junctionRoadShields;
};

struct RoadInfo
{
  std::string m_formattedName;
  RoadShieldInfo m_shields;
  std::string m_name;            // E.g. "Bayshore Freeway".
  std::string m_ref;             // Road number, e.g. "CA 85".
  std::string m_junctionRef;     // Junction/exit number, e.g. "6A".
  std::string m_destinationRef;  // Number of the road the exit leads to, e.g. "US 101 South".
  std::string m_destination;     // E.g. "San Jose; San Francisco International Airport".
  bool m_isLink = false;         // True when the next segment is a link (ramp).
};
}  // namespace routing
