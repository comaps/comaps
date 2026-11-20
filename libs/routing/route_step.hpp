#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "platform/distance.hpp"
#include "routing/lanes/lane_info.hpp"
#include "routing/road_info.hpp"
#include "routing/turns.hpp"

namespace routing
{

struct RouteStepInfo
{
  uint32_t m_index = 0;
  uint32_t m_sequence = 0;
  turns::CarDirection m_turn = turns::CarDirection::None;
  turns::PedestrianDirection m_pedestrianTurn = turns::PedestrianDirection::None;
  std::string m_fromStreetName;
  RoadInfo m_toRoad;
  turns::lanes::LanesInfo m_lanes;
  uint32_t m_exitNum = 0;
  platform::Distance m_formattedDistance;
  double m_distMeters = 0.0;
  double m_distFromStartMeters = 0.0;
  std::string m_textualInstruction;
};

// A versioned route plan (which is a list of RouteStepInfo)
struct RoutePlan
{
  uint64_t m_routeId = 0;

  // The sequence number of each RouteStepInfo will be the index of each step in this array
  std::vector<RouteStepInfo> m_steps;
};

}  // namespace routing
