#pragma once

#include <string>
#include "platform/distance.hpp"
#include "routing/turns.hpp"

namespace routing
{

struct RouteStepInfo
{
  uint32_t m_index = 0;
  turns::CarDirection m_turn = turns::CarDirection::None;
  turns::PedestrianDirection m_pedestrianTurn = turns::PedestrianDirection::None;
  std::string m_fromStreetName;
  std::string m_toStreetName;
  uint32_t m_exitNum = 0;
  platform::Distance m_formattedDistance;
  double m_distMeters = 0.0;
  std::string m_textualInstruction;
};

}  // namespace routing
