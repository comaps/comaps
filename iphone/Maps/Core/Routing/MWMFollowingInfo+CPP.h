#pragma once

#include "routing/following_info.hpp"

#include <string>

namespace routing::ios
{
struct FollowingRoadInfo
{
  std::string const & m_formattedName;
  FollowingInfo::RoadShieldInfo const & m_shields;
  std::string const & m_name;
  std::string const & m_ref;
  std::string const & m_junctionRef;
  std::string const & m_destinationRef;
  std::string const & m_destination;
  bool m_isLink;
};

inline bool IsCombinedRoundaboutEntrance(FollowingInfo const & info)
{
  return info.m_turn == turns::CarDirection::EnterRoundAbout;
}

inline FollowingRoadInfo GetDisplayedRoadInfo(FollowingInfo const & info)
{
  if (IsCombinedRoundaboutEntrance(info))
  {
    return {info.m_nextNextStreetName,
            info.m_nextNextStreetShields,
            info.m_nextNextName,
            info.m_nextNextRef,
            info.m_nextNextJunctionRef,
            info.m_nextNextDestinationRef,
            info.m_nextNextDestination,
            info.m_nextNextIsLink};
  }

  return {info.m_nextStreetName,
          info.m_nextStreetShields,
          info.m_nextName,
          info.m_nextRef,
          info.m_nextJunctionRef,
          info.m_nextDestinationRef,
          info.m_nextDestination,
          info.m_nextIsLink};
}
}  // namespace routing::ios
