#pragma once

#include <string>
#include "platform/distance.hpp"
#include "routing/lanes/lane_info.hpp"
#include "routing/turns.hpp"

namespace routing
{

struct RouteStepInfo
{
  uint32_t m_index = 0;
  uint32_t m_turnIndex = 0;
  turns::CarDirection m_turn = turns::CarDirection::None;
  turns::PedestrianDirection m_pedestrianTurn = turns::PedestrianDirection::None;
  std::string m_fromStreetName;
  std::string m_toStreetName;
  std::string m_toRef;
  std::string m_toJunctionRef;
  std::string m_toDestinationRef;
  std::string m_toDestination;
  bool m_toIsLink = false;
  turns::lanes::LanesInfo m_lanes;
  uint32_t m_exitNum = 0;
  platform::Distance m_formattedDistance;
  double m_distMeters = 0.0;
  std::string m_textualInstruction;
};

}  // namespace routing
