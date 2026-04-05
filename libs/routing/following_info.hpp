#pragma once

#include "platform/distance.hpp"

#include "routing/lanes/lane_info.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/turns.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
class FollowingInfo
{
public:
  FollowingInfo()
    : m_turn(turns::CarDirection::None)
    , m_nextTurn(turns::CarDirection::None)
    , m_exitNum(0)
    , m_time(0)
    , m_completionPercent(0)
    , m_pedestrianTurn(turns::PedestrianDirection::None)
    , m_routingSessionState(routing::SessionState::NoValidRoute)
  {}

  bool IsValid() const { return m_distToTarget.IsValid(); }

  /// @name Formatted covered distance.
  platform::Distance m_distToTarget;

  /// @name Formatted distance to the next turn.
  //@{
  platform::Distance m_distToTurn;
  turns::CarDirection m_turn;
  /// Turn after m_turn. Returns NoTurn if there is no turns after.
  turns::CarDirection m_nextTurn;
  uint32_t m_exitNum;
  //@}
  int m_time;
  /// Contains lane information on the edge before the turn.
  turns::lanes::LanesInfo m_lanes;
  // m_turnNotifications contains information about the next turn notifications.
  // If there is nothing to pronounce m_turnNotifications is empty.
  // If there is something to pronounce the size of m_turnNotifications may be one or even more
  // depends on the number of notifications to pronounce.
  std::vector<std::string> m_turnNotifications;
  // Current street name. May be empty.
  std::string m_currentStreetName;
  // The next street name. May be empty.
  std::string m_nextStreetName;
  // The next next street name. May be empty.
  std::string m_nextNextStreetName;

  // Percentage of the route completion.
  double m_completionPercent;

  /// @name Pedestrian direction information
  //@{
  turns::PedestrianDirection m_pedestrianTurn;
  //@}

  // Current speed limit in meters per second.
  // If no info about speed limit then m_speedLimitMps < 0.
  double m_speedLimitMps = -1.0;

  // Routing state.
  SessionState m_routingSessionState;

  // Index of the next intermediate stop:
  //  -1 = invalid next intermediate stops.
  //   0 = there are no next intermediate stops.
  //   1 = intermediate stop #1.
  //   2 = intermediate stop #2.
  //   and so on...
  int m_indexOfNextStop;

  // Time & distance information to next intermediate stop.
  platform::Distance m_distToNextStop;
  int m_timeToNextStop;
};
}  // namespace routing
