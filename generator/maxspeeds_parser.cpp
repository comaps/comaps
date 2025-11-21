#include "generator/maxspeeds_parser.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <cctype>
#include <limits>
#include <unordered_map>

namespace generator
{
using measurement_utils::Units;

static std::unordered_map<std::string, routing::SpeedInUnits> const kRoadCategoryToSpeed = {
    {"AR:motorway", {130, Units::Metric}},
    {"AR:rural", {110, Units::Metric}},
    {"AR:urban", {40, Units::Metric}},
    {"AR:urban:primary", {60, Units::Metric}},
    {"AR:urban:secondary", {60, Units::Metric}},
    {"AT:bicycle_road", {30, Units::Metric}},
    {"AT:motorway", {130, Units::Metric}},
    {"AT:rural", {100, Units::Metric}},
    {"AT:shared_zone30", {30, Units::Metric}},
    {"AT:shared_zone20", {20, Units::Metric}},
    {"AT:trunk", {100, Units::Metric}},
    {"AT:urban", {50, Units::Metric}},
    {"AT:zone40", {40, Units::Metric}},
    {"AT:zone30", {30, Units::Metric}},
    {"AT:zone20", {20, Units::Metric}},
    {"AU:rural", {100, Units::Metric}},
    {"AU:urban", {50, Units::Metric}},
    {"BE-BRU:rural", {70, Units::Metric}},
    {"BE-BRU:urban", {30, Units::Metric}},
    {"BE:cyclestreet", {30, Units::Metric}},
    {"BE:living_street", {20, Units::Metric}},
    {"BE:motorway", {120, Units::Metric}},
    {"BE:rural", {70, Units::Metric}},
    {"BE:school", {30, Units::Metric}},
    {"BE:trunk", {120, Units::Metric}},
    {"BE:urban", {50, Units::Metric}},
    {"BE-VLG:rural", {70, Units::Metric}},
    {"BE-VLG:urban", {50, Units::Metric}},
    {"BE-WAL:rural", {90, Units::Metric}},
    {"BE-WAL:urban", {50, Units::Metric}},
    {"BE:zone30", {30, Units::Metric}},
    {"BG:living_street", {20, Units::Metric}},
    {"BG:motorway", {140, Units::Metric}},
    {"BG:rural", {90, Units::Metric}},
    {"BG:trunk", {120, Units::Metric}},
    {"BG:urban", {50, Units::Metric}},
    {"BY:living_street", {20, Units::Metric}},
    {"BY:motorway", {110, Units::Metric}},
    {"BY:rural", {90, Units::Metric}},
    {"BY:urban", {60, Units::Metric}},
    {"CA-BC:rural", {80, Units::Metric}},
    {"CA-BC:urban", {50, Units::Metric}},
    {"CA-MB:rural", {90, Units::Metric}},
    {"CA-MB:urban", {50, Units::Metric}},
    {"CA-ON:rural", {80, Units::Metric}},
    {"CA-ON:urban", {50, Units::Metric}},
    {"CA-QC:motorway", {100, Units::Metric}},
    {"CA-QC:urban", {50, Units::Metric}},
    {"CH:motorway", {120, Units::Metric}},
    {"CH:rural", {80, Units::Metric}},
    {"CH:trunk", {100, Units::Metric}},
    {"CH:urban", {50, Units::Metric}},
    {"CH:zone30", {30, Units::Metric}},
    {"CZ:living_street", {20, Units::Metric}},
    {"CZ:motorway", {130, Units::Metric}},
    {"CZ:pedestrian_zone", {20, Units::Metric}},
    {"CZ:rural", {90, Units::Metric}},
    {"CZ:trunk", {110, Units::Metric}},
    {"CZ:urban", {50, Units::Metric}},
    {"CZ:urban_motorway", {80, Units::Metric}},
    {"CZ:urban_trunk", {80, Units::Metric}},
    {"CZ:zone30", {30, Units::Metric}},
    {"DE:bicycle_road", {30, Units::Metric}},
    {"DE:living_street", {7, Units::Metric}},
    {"DE:motorway", {routing::kNoneMaxSpeed, Units::Metric}},
    {"DE:rural", {100, Units::Metric}},
    {"DE:trunk", {routing::kNoneMaxSpeed, Units::Metric}},
    {"DE:urban", {50, Units::Metric}},
    {"DE:zone30", {30, Units::Metric}},
    {"DE:zone20", {20, Units::Metric}},
    {"DK:motorway", {130, Units::Metric}},
    {"DK:rural", {80, Units::Metric}},
    {"DK:urban", {50, Units::Metric}},
    {"EE:rural", {90, Units::Metric}},
    {"EE:urban", {50, Units::Metric}},
    {"ES:living_street", {20, Units::Metric}},
    {"ES:motorway", {120, Units::Metric}},
    {"ES:rural", {90, Units::Metric}},
    {"ES:trunk", {100, Units::Metric}},
    {"ES:urban", {50, Units::Metric}},
    {"ES:zone30", {30, Units::Metric}},
    {"FI:living_street", {20, Units::Metric}},
    {"FI:motorway", {120, Units::Metric}},
    {"FI:rural", {80, Units::Metric}},
    {"FI:trunk", {100, Units::Metric}},
    {"FI:urban", {50, Units::Metric}},
    {"FI:zone50", {50, Units::Metric}},
    {"FI:zone40", {40, Units::Metric}},
    {"FI:zone30", {30, Units::Metric}},
    {"FI:zone20", {20, Units::Metric}},
    {"FR:motorway", {130, Units::Metric}},
    {"FR:trunk", {110, Units::Metric}},
    {"FR:rural", {80, Units::Metric}},
    {"FR:urban", {50, Units::Metric}},
    {"FR:zone30", {30, Units::Metric}},
    {"FR:living_street", {20, Units::Metric}},
    {"GB:motorway", {70, Units::Imperial}},    // 70 mph = 112.65408 kmph
    {"GB:nsl_dual", {70, Units::Imperial}},    // 70 mph = 112.65408 kmph
    {"GB:nsl_restricted", {30, Units::Imperial}},
    {"GB:nsl_single", {60, Units::Imperial}},  // 60 mph = 96.56064 kmph
    {"GB-WLS:nsl_restricted", {20, Units::Imperial}},
    {"GB:zone40", {40, Units::Imperial}},
    {"GB:zone20", {20, Units::Imperial}},
    {"GR:living_street", {20, Units::Metric}},
    {"GR:motorway", {130, Units::Metric}},
    {"GR:trunk", {110, Units::Metric}},
    {"GR:rural", {90, Units::Metric}},
    {"GR:urban", {50, Units::Metric}},
    {"HU:motorway", {130, Units::Metric}},
    {"HU:trunk", {110, Units::Metric}},
    {"HU:rural", {90, Units::Metric}},
    {"HU:urban", {50, Units::Metric}},
    {"HU:living_street", {20, Units::Metric}},
    {"ID:motorway", {100, Units::Metric}},
    {"ID:rural", {80, Units::Metric}},
    {"ID:urban", {50, Units::Metric}},
    {"ID:residential", {30, Units::Metric}},
    {"IL:motorway", {110, Units::Metric}},
    {"IL:trunk", {100, Units::Metric}},
    {"IL:urban", {50, Units::Metric}},
    {"IL:living_street", {30, Units::Metric}},
    {"IN:motorway", {120, Units::Metric}},
    {"IN:rural", {70, Units::Metric}},
    {"IN:urban", {70, Units::Metric}},
    {"IT:motorway", {130, Units::Metric}},
    {"IT:trunk", {110, Units::Metric}},
    {"IT:rural", {90, Units::Metric}},
    {"IT:urban", {50, Units::Metric}},
    {"JP:express", {100, Units::Metric}},
    {"JP:motorway", {100, Units::Metric}},
    {"JP:national", {60, Units::Metric}},
    {"JP:nsl", {60, Units::Metric}},
    {"KR:motorway", {80, Units::Metric}},
    {"KR:rural", {60, Units::Metric}},
    {"KR:trunk", {80, Units::Metric}},
    {"KR:urban", {50, Units::Metric}},
    {"LT:motorway", {110, Units::Metric}},
    {"LT:rural", {90, Units::Metric}},
    {"LT:trunk", {110, Units::Metric}},
    {"LT:urban", {50, Units::Metric}},
    {"LT:living_street", {20, Units::Metric}},
    {"NL:living_street", {15, Units::Metric}},
    {"NL:motorway", {130, Units::Metric}},
    {"NL:motorroad", {100, Units::Metric}},
    {"NL:rural", {80, Units::Metric}},
    {"NL:urban", {50, Units::Metric}},
    {"NL:zone60", {60, Units::Metric}},
    {"NL:zone30", {30, Units::Metric}},
    {"NO:rural", {80, Units::Metric}},
    {"NO:urban", {50, Units::Metric}},
    {"ON:rural", {80, Units::Metric}},
    {"ON:urban", {50, Units::Metric}},
    {"PH:motorway", {100, Units::Metric}},
    {"PH:rural", {80, Units::Metric}},
    {"PH:urban", {40, Units::Metric}},
    {"PL:motorway", {140, Units::Metric}},
	{"PL:trunk", {100, Units::Metric}},
    {"PL:rural", {90, Units::Metric}},
    {"PL:urban", {50, Units::Metric}},
    {"PL:living_street", {20, Units::Metric}},
    {"PT:motorway", {120, Units::Metric}},
    {"PT:trunk", {100, Units::Metric}},
    {"PT:rural", {90, Units::Metric}},
    {"PT:urban", {50, Units::Metric}},
    {"RO:motorway", {130, Units::Metric}},
    {"RO:trunk", {100, Units::Metric}},
    {"RO:rural", {90, Units::Metric}},
    {"RO:urban", {50, Units::Metric}},
    {"RS:living_street", {10, Units::Metric}},
    {"RS:motorway", {130, Units::Metric}},
    {"RS:trunk", {100, Units::Metric}},
    {"RS:rural", {80, Units::Metric}},
    {"RS:urban", {50, Units::Metric}},
    {"RU:motorway", {110, Units::Metric}},
    {"RU:rural", {90, Units::Metric}},
    {"RU:urban", {60, Units::Metric}},
    {"RU:living_street", {20, Units::Metric}},
    {"SE:motorway", {110, Units::Metric}},
    {"SE:rural", {70, Units::Metric}},
    {"SE:trunk", {90, Units::Metric}},
    {"SE:urban", {50, Units::Metric}},
    {"SI:motorway", {130, Units::Metric}},
    {"SI:trunk", {110, Units::Metric}},
    {"SI:rural", {90, Units::Metric}},
    {"SI:urban", {50, Units::Metric}},
    {"SK:motorway", {130, Units::Metric}},
    {"SK:motorway_urban", {90, Units::Metric}},
    {"SK:trunk", {90, Units::Metric}},
    {"SK:rural", {90, Units::Metric}},
    {"SK:urban", {50, Units::Metric}},
    {"SK:living_street", {20, Units::Metric}},
    {"SL:motorway", {130, Units::Metric}},
    {"SL:trunk", {110, Units::Metric}},
    {"SL:rural", {90, Units::Metric}},
    {"SL:urban", {50, Units::Metric}},
    {"TR:trunk", {110, Units::Metric}},
    {"TR:rural", {90, Units::Metric}},
    {"TR:urban", {50, Units::Metric}},
    {"TR:zone30", {30, Units::Metric}},
    {"TR:living_street", {20, Units::Metric}},
    {"UA:motorway", {130, Units::Metric}},
    {"UA:trunk", {110, Units::Metric}},
    {"UA:rural", {90, Units::Metric}},
    {"UA:urban", {50, Units::Metric}},
    {"UA:living_street", {20, Units::Metric}},
    {"UK:motorway", {70, Units::Imperial}},    // 70 mph
    {"UK:nsl_dual", {70, Units::Imperial}},    // 70 mph
    {"UK:nsl_single", {60, Units::Imperial}},  // 60 mph
    {"UZ:living_street", {30, Units::Metric}},
    {"UZ:motorway", {110, Units::Metric}},
    {"UZ:rural", {100, Units::Metric}},
    {"UZ:urban", {70, Units::Metric}},
    {"ZA:motorway", {120, Units::Metric}},
    {"ZA:rural", {100, Units::Metric}},
    {"ZA:urban", {60, Units::Metric}},
};

bool RoadCategoryToSpeed(std::string const & category, routing::SpeedInUnits & speed)
{
  auto const it = kRoadCategoryToSpeed.find(category);
  if (it == kRoadCategoryToSpeed.cend())
    return false;

  speed = it->second;
  return true;
}

bool ParseMaxspeedTag(std::string const & maxspeedValue, routing::SpeedInUnits & speed)
{
  if (RoadCategoryToSpeed(maxspeedValue, speed))
    return true;

  if (maxspeedValue == "none")
  {
    speed.SetSpeed(routing::kNoneMaxSpeed);
    speed.SetUnits(Units::Metric);  // It's dummy value in case of kNoneMaxSpeed
    return true;
  }

  if (maxspeedValue == "walk")
  {
    speed.SetSpeed(routing::kWalkMaxSpeed);
    speed.SetUnits(Units::Metric);  // It's dummy value in case of kWalkMaxSpeed
    return true;
  }

  // strings::to_int doesn't work here because of bad errno.
  std::string speedStr;
  size_t i;
  for (i = 0; i < maxspeedValue.size(); ++i)
  {
    if (!isdigit(maxspeedValue[i]))
      break;

    speedStr += maxspeedValue[i];
  }

  while (i < maxspeedValue.size() && strings::IsASCIISpace(maxspeedValue[i]))
    ++i;

  if (maxspeedValue.size() == i || maxspeedValue.substr(i).starts_with("kmh"))
  {
    uint64_t kmph = 0;
    if (!strings::to_uint64(speedStr.c_str(), kmph) || kmph == 0 || kmph > std::numeric_limits<uint16_t>::max())
      return false;

    speed.SetSpeed(static_cast<uint16_t>(kmph));
    speed.SetUnits(Units::Metric);
    return true;
  }

  if (maxspeedValue.substr(i).starts_with("mph"))
  {
    uint64_t mph = 0;
    if (!strings::to_uint64(speedStr.c_str(), mph) || mph == 0 || mph > std::numeric_limits<uint16_t>::max())
      return false;

    speed.SetSpeed(static_cast<uint16_t>(mph));
    speed.SetUnits(Units::Imperial);
    return true;
  }

  return false;
}

std::string UnitsToString(Units units)
{
  switch (units)
  {
  case Units::Metric: return "Metric";
  case Units::Imperial: return "Imperial";
  }
  UNREACHABLE();
}

Units StringToUnits(std::string_view units)
{
  if (units == "Metric")
    return Units::Metric;
  if (units == "Imperial")
    return Units::Imperial;

  CHECK(false, (units));
  return Units::Metric;
}
}  // namespace generator
