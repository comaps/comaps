#include "traffxml/traff_model_xml.hpp"
#include "traffxml/traff_model.hpp"

#include "base/logging.hpp"

#include <cstring>
#include <iomanip>
#include <optional>
#include <type_traits>
#include <utility>

#include <boost/bimap.hpp>
#include <boost/regex.hpp>

#include <pugixml.hpp>

using namespace std;

namespace traffxml
{
/**
 * @brief Creates and initializes a `boost::bimap`.
 *
 * @param list A braced initializer list of left-right tuples.
 * @return A new bimap instance of the tuples in `list`.
 */
template <typename L, typename R>
boost::bimap<L, R>
MakeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list)
{
  return boost::bimap<L, R>(list.begin(), list.end());
}

const boost::bimap<std::string, Directionality> kDirectionalityMap = MakeBimap<std::string, Directionality>({
  {"ONE_DIRECTION", Directionality::OneDirection},
  {"BOTH_DIRECTIONS", Directionality::BothDirections}
});

const boost::bimap<std::string, Fuzziness> kFuzzinessMap = MakeBimap<std::string, Fuzziness>({
  {"LOW_RES", Fuzziness::LowRes},
  {"MEDIUM_RES", Fuzziness::MediumRes},
  {"END_UNKNOWN", Fuzziness::EndUnknown},
  {"START_UNKNOWN", Fuzziness::StartUnknown},
  {"EXTENT_UNKNOWN", Fuzziness::ExtentUnknown}
});

const boost::bimap<std::string, Ramps> kRampsMap = MakeBimap<std::string, Ramps>({
  {"ALL_RAMPS", Ramps::All},
  {"ENTRY_RAMP", Ramps::Entry},
  {"EXIT_RAMP", Ramps::Exit},
  {"NONE", Ramps::None}
});

const boost::bimap<std::string, traffxml::RoadClass> kRoadClassMap = MakeBimap<std::string, RoadClass>({
  {"MOTORWAY", RoadClass::Motorway},
  {"TRUNK", RoadClass::Trunk},
  {"PRIMARY", RoadClass::Primary},
  {"SECONDARY", RoadClass::Secondary},
  {"TERTIARY", RoadClass::Tertiary},
  {"OTHER", RoadClass::Other}
});

const boost::bimap<std::string, EventClass> kEventClassMap = MakeBimap<std::string, EventClass>({
  {"INVALID", EventClass::Invalid},
  {"ACTIVITY", EventClass::Activity},
  {"AUTHORITY", EventClass::Authority},
  {"CARPOOL", EventClass::Carpool},
  {"CONGESTION", EventClass::Congestion},
  {"CONSTRUCTION", EventClass::Construction},
  {"DELAY", EventClass::Delay},
  {"ENVIRONMENT", EventClass::Environment},
  {"EQUIPMENT_STATUS", EventClass::EquipmentStatus},
  {"HAZARD", EventClass::Hazard},
  {"INCIDENT", EventClass::Incident},
  {"RESTRICTION", EventClass::Restriction},
  {"SECURITY", EventClass::Security},
  {"TRANSPORT", EventClass::Transport},
  {"WEATHER", EventClass::Weather}
});

const boost::bimap<std::string, EventType> kEventTypeMap = MakeBimap<std::string, EventType>({
  {"INVALID", EventType::Invalid},
  // TODO Activity*, Authority*, Carpool* (not in enum yet)
  {"CONGESTION_CLEARED", EventType::CongestionCleared},
  {"CONGESTION_FORECAST_WITHDRAWN", EventType::CongestionForecastWithdrawn},
  {"CONGESTION_HEAVY_TRAFFIC", EventType::CongestionHeavyTraffic},
  {"CONGESTION_LONG_QUEUE", EventType::CongestionLongQueue},
  {"CONGESTION_NONE", EventType::CongestionNone},
  {"CONGESTION_NORMAL_TRAFFIC", EventType::CongestionNormalTraffic},
  {"CONGESTION_QUEUE", EventType::CongestionQueue},
  {"CONGESTION_QUEUE_LIKELY", EventType::CongestionQueueLikely},
  {"CONGESTION_SLOW_TRAFFIC", EventType::CongestionSlowTraffic},
  {"CONGESTION_STATIONARY_TRAFFIC", EventType::CongestionStationaryTraffic},
  {"CONGESTION_STATIONARY_TRAFFIC_LIKELY", EventType::CongestionStationaryTrafficLikely},
  {"CONGESTION_TRAFFIC_BUILDING_UP", EventType::CongestionTrafficBuildingUp},
  {"CONGESTION_TRAFFIC_CONGESTION", EventType::CongestionTrafficCongestion},
  {"CONGESTION_TRAFFIC_EASING", EventType::CongestionTrafficEasing},
  {"CONGESTION_TRAFFIC_FLOWING_FREELY", EventType::CongestionTrafficFlowingFreely},
  {"CONGESTION_TRAFFIC_HEAVIER_THAN_NORMAL", EventType::CongestionTrafficHeavierThanNormal},
  {"CONGESTION_TRAFFIC_LIGHTER_THAN_NORMAL", EventType::CongestionTrafficLighterThanNormal},
  {"CONGESTION_TRAFFIC_MUCH_HEAVIER_THAN_NORMAL", EventType::CongestionTrafficMuchHeavierThanNormal},
  {"CONGESTION_TRAFFIC_PROBLEM", EventType::CongestionTrafficProblem},
  // TODO Construction* (not in enum yet)
  {"DELAY_CLEARANCE", EventType::DelayClearance},
  {"DELAY_DELAY", EventType::DelayDelay},
  {"DELAY_DELAY_POSSIBLE", EventType::DelayDelayPossible},
  {"DELAY_FORECAST_WITHDRAWN", EventType::DelayForecastWithdrawn},
  {"DELAY_LONG_DELAY", EventType::DelayLongDelay},
  {"DELAY_SEVERAL_HOURS", EventType::DelaySeveralHours},
  {"DELAY_UNCERTAIN_DURATION", EventType::DelayUncertainDuration},
  {"DELAY_VERY_LONG_DELAY", EventType::DelayVeryLongDelay},
  // TODO Environment*, EquipmentStatus*, Hazard*, Incident* (not in enum yet)
  // TODO complete Restriction* (not in enum yet)
  {"RESTRICTION_BLOCKED", EventType::RestrictionBlocked},
  {"RESTRICTION_BLOCKED_AHEAD", EventType::RestrictionBlockedAhead},
  {"RESTRICTION_CARRIAGEWAY_BLOCKED", EventType::RestrictionCarriagewayBlocked},
  {"RESTRICTION_CARRIAGEWAY_CLOSED", EventType::RestrictionCarriagewayClosed},
  {"RESTRICTION_CLOSED", EventType::RestrictionClosed},
  {"RESTRICTION_CLOSED_AHEAD", EventType::RestrictionClosedAhead},
  {"RESTRICTION_ENTRY_BLOCKED", EventType::RestrictionEntryBlocked},
  {"RESTRICTION_ENTRY_REOPENED", EventType::RestrictionEntryReopened},
  {"RESTRICTION_EXIT_BLOCKED", EventType::RestrictionExitBlocked},
  {"RESTRICTION_EXIT_REOPENED", EventType::RestrictionExitReopened},
  {"RESTRICTION_OPEN", EventType::RestrictionOpen},
  {"RESTRICTION_RAMP_BLOCKED", EventType::RestrictionRampBlocked},
  {"RESTRICTION_RAMP_CLOSED", EventType::RestrictionRampClosed},
  {"RESTRICTION_RAMP_REOPENED", EventType::RestrictionRampReopened},
  {"RESTRICTION_REOPENED", EventType::RestrictionReopened},
  {"RESTRICTION_SPEED_LIMIT", EventType::RestrictionSpeedLimit},
  {"RESTRICTION_SPEED_LIMIT_LIFTED", EventType::RestrictionSpeedLimitLifted},
  // TODO Security*, Transport*, Weather* (not in enum yet)
});

const boost::bimap<std::string, traffic::SpeedGroup> kSpeedGroupMap = MakeBimap<std::string, traffic::SpeedGroup>({
  {"G0", traffic::SpeedGroup::G0},
  {"G1", traffic::SpeedGroup::G1},
  {"G2", traffic::SpeedGroup::G2},
  {"G3", traffic::SpeedGroup::G3},
  {"G4", traffic::SpeedGroup::G4},
  {"G5", traffic::SpeedGroup::G5},
  {"TEMP_BLOCK", traffic::SpeedGroup::TempBlock},
  {"UNKNOWN", traffic::SpeedGroup::Unknown}
});

/**
 * @brief Retrieves an integer value from an attribute.
 *
 * @param attribute The XML attribute to retrieve.
 * @param value The variable which will receive the value, must be of an integer type
 * @return `true` on success, `false` if the attribute is not set or does not contain an integer value.
 */
template <typename Value>
bool IntegerFromXml(pugi::xml_attribute const & attribute, Value & value)
{
  if (attribute.empty())
    return false;
  try
  {
    value = static_cast<Value>(is_signed<Value>::value
                               ? std::stoll(attribute.as_string())
                               : std::stoull(attribute.as_string()));
    return true;
  }
  catch (std::invalid_argument const& ex)
  {
    return false;
  }
}

/**
 * @brief Retrieves an integer value from an attribute.
 *
 * @param attribute The XML attribute to retrieve.
 * @return `true` on success, `false` if the attribute is not set or does not contain an integer value.
 */
std::optional<int> OptionalIntegerFromXml(pugi::xml_attribute const & attribute)
{
  if (attribute.empty())
    return std::nullopt;
  try
  {
    int result = std::stoi(attribute.as_string());
    return result;
  }
  catch (std::invalid_argument const& ex)
  {
    return std::nullopt;
  }
}

/**
 * @brief Retrieves a float value from an attribute.
 *
 * @param attribute The XML attribute to retrieve.
 * @return `true` on success, `false` if the attribute is not set or does not contain a float value.
 */
std::optional<float> OptionalFloatFromXml(pugi::xml_attribute const & attribute)
{
  if (attribute.empty())
    return std::nullopt;
  try
  {
    float result = std::stof(attribute.as_string());
    return result;
  }
  catch (std::invalid_argument const& ex)
  {
    return std::nullopt;
  }
}

/**
 * @brief Retrieves a string from an attribute.
 *
 * @param attribute The XML attribute to retrieve.
 * @param string Receives the string retrieved.
 * @return `true` on success, `false` if the attribute is not set or set to an empty string.
 */
bool StringFromXml(pugi::xml_attribute const & attribute, std::string & string)
{
  if (attribute.empty())
    return false;
  string = attribute.as_string();
  return true;
}

/**
 * @brief Retrieves a string from an XML element.
 *
 * @param node The XML element to retrieve.
 * @param string Receives the string retrieved.
 * @return `true` on success, `false` if the node does not exist.
 */
bool StringFromXml(pugi::xml_node const & node, std::string & string)
{
  if (!node)
    return false;
  string = node.text().as_string();
  return true;
}

/**
 * @brief Retrieves a string from an attribute.
 *
 * @param attribute The XML attribute to retrieve.
 * @return The string, or `std::nullopt` if the attribute is not set or set to an empty string.
 */
std::optional<std::string> OptionalStringFromXml(pugi::xml_attribute const & attribute)
{
  std::string result;
  if (!StringFromXml(attribute, result))
    return std::nullopt;
  return result;
}

/**
 * @brief Parses time in ISO 8601 format from a time attribute and stores it in an `IsoTime`.
 *
 * ISO 8601 timestamps have the format `yyyy-mm-ddThh:mm:ss[.sss]`, optionally followed by a UTC
 * offset. For example, `2019-11-01T11:45:42+01:00` refers to 11:45:42 in the UTC+1 timezone, which
 * is 10:45:42 UTC.
 *
 * A UTC offset of `Z` denotes UTC and is equivalent to `+00:00` or `-00:00`. UTC is also assumed
 * if no UTC offset is specified.
 *
 * The UTC offset can be specified as `hh:mm`, `hhmm` or `hh`.
 *
 * Seconds can be specified as integer or float, but will be rounded to the nearest integer. For
 * example, 42.645 seconds will be rounded to 43 seconds.
 *
 * @param attribute The XML attribute from which to receive time.
 * @param tm Receives the parsed time.
 * @return `true` on success, `false` if the attribute is not set or does not contain a timestamp.
 */
bool TimeFromXml(pugi::xml_attribute const & attribute, IsoTime & tm)
{
  std::string timeString;
  if (!StringFromXml(attribute, timeString))
    return false;

  std::optional<IsoTime> result = IsoTime::ParseIsoTime(timeString);
  if (!result)
    return false;

  tm = result.value();
  return true;
}

/**
 * @brief Parses time in ISO 8601 format from a time attribute and stores it in an `IsoTime`.
 *
 * ISO 8601 timestamps have the format `yyyy-mm-ddThh:mm:ss[.sss]`, optionally followed by a UTC
 * offset. For example, `2019-11-01T11:45:42+01:00` refers to 11:45:42 in the UTC+1 timezone, which
 * is 10:45:42 UTC.
 *
 * A UTC offset of `Z` denotes UTC and is equivalent to `+00:00` or `-00:00`. UTC is also assumed
 * if no UTC offset is specified.
 *
 * The UTC offset can be specified as `hh:mm`, `hhmm` or `hh`.
 *
 * Seconds can be specified as integer or float, but will be rounded to the nearest integer. For
 * example, 42.645 seconds will be rounded to 43 seconds.
 *
 * @param attribute The XML attribute from which to receive time.
 * @return The parsed time, or `std::nullopt` if the attribute is not set or does not contain a timestamp.
 */
std::optional<IsoTime> OptionalTimeFromXml(pugi::xml_attribute const & attribute)
{
  IsoTime result = IsoTime::Now();
  if (!TimeFromXml(attribute, result))
    return std::nullopt;
  return result;
}

/**
 * @brief Retrieves a response status from an attribute.
 *
 * @param attribute The XML attribute to retrieve.
 * @param status Receives the status retrieved.
 * @return `true` on success, `false` if the attribute is not set or set to an empty string.
 */
bool ResponseStatusFromXml(pugi::xml_attribute const & attribute, ResponseStatus & status)
{
  std::string statusString;
  if (!StringFromXml(attribute, statusString))
    return false;

  if (statusString == "OK")
    status = ResponseStatus::Ok;
  else if (statusString == "INVALID")
    status = ResponseStatus::InvalidOperation;
  else if (statusString == "SUBSCRIPTION_REJECTED")
    status = ResponseStatus::SubscriptionRejected;
  else if (statusString == "NOT_COVERED")
    status = ResponseStatus::NotCovered;
  else if (statusString == "PARTIALLY_COVERED")
    status = ResponseStatus::PartiallyCovered;
  else if (statusString == "SUBSCRIPTION_UNKNOWN")
    status = ResponseStatus::SubscriptionUnknown;
  else if (statusString == "PUSH_REJECTED")
    status = ResponseStatus::PushRejected;
  else if (statusString == "INTERNAL_ERROR")
    status = ResponseStatus::InternalError;
  else
    status = ResponseStatus::Invalid;

  return true;
}

/**
 * @brief Retrieves a boolean value from an attribute.
 * @param attribute The XML attribute to retrieve.
 * @param defaultValue The default value to return.
 * @return The value of the attribute, or `defaultValue` if the attribute is not set.
 */
bool BoolFromXml(pugi::xml_attribute const & attribute, bool defaultValue)
{
  if (attribute.empty())
    return defaultValue;
  return attribute.as_bool();
}

/**
 * @brief Retrieves a boolean value from an attribute.
 * @param attribute The XML attribute to retrieve.
 * @return The value of the attribute, or `std::nullopt` if the attribute is not set.
 */
std::optional<bool> OptionalBoolFromXml(pugi::xml_attribute const & attribute)
{
  if (attribute.empty())
    return std::nullopt;
  return attribute.as_bool();
}

/**
 * @brief Retrieves an enum value from an attribute.
 *
 * Enum values are retrieved in two steps: first, a string is retrieved, which is then decoded to
 * an enum value. The enum type is determined by the type of `value` and the value type of `map`,
 * both of which must match. The mapping between strings and their corresponding enum values is
 * determined by the entries in `map`.
 *
 * @param attribute The XML attribute to retrieve.
 * @param Value Receives the enum value to retrieve.
 * @param map A map from strings to their respective enum values.
 * @return `true` on success, `false` if the attribute is not set or its value is not found in `map`.
 */
template <typename Value>
bool EnumFromXml(pugi::xml_attribute const & attribute, Value & value,
                 boost::bimap<std::string, Value> const & map)
{
  std::string string;
  if (StringFromXml(attribute, string))
  {
    auto it = map.left.find(string);
    if (it != map.left.end())
    {
      value = it->second;
      return true;
    }
    else
      LOG(LWARNING, ("Unknown value for", attribute.name(), ":", string, "(ignoring)"));
  }
  return false;
}

/**
 * @brief Retrieves an enum value from an attribute.
 *
 * Enum values are retrieved in two steps: first, a string is retrieved, which is then decoded to
 * an enum value. The enum type is determined by the value type of `map`. The mapping between
 * strings and their corresponding enum values is determined by the entries in `map`.
 *
 * @param attribute The XML attribute to retrieve.
 * @param map A map from strings to their respective enum values.
 * @return The enum value, or `std::nullopt` if the attribute is not set or its value is not found in `map`.
 */
template <typename Value>
std::optional<Value> OptionalEnumFromXml(pugi::xml_attribute const & attribute,
                                         boost::bimap<std::string, Value> const & map)
{
  std::string string;
  if (StringFromXml(attribute, string))
  {
    auto it = map.left.find(string);
    if (it != map.left.end())
      return it->second;
    else
      LOG(LWARNING, ("Unknown value for", attribute.name(), ":", string, "(ignoring)"));
  }
  return std::nullopt;
}

/**
 * @brief Stores an enum value in an attribute.
 *
 * The enum value is translated into a string using `map`. An attribute named `name` is then added
 * to `node`, with the translated value.
 *
 * @param value The enum value.
 * @param name The name of the attribute to store the value in.
 * @param node The node to which the attribute will be added.
 * @param map A map between strings and their respective enum values.
 */
template <typename Value>
void EnumToXml(Value const & value, std::string name, pugi::xml_node & node, boost::bimap<std::string, Value> const & map)
{
  auto it = map.right.find(value);
  if (it != map.right.end())
    node.append_attribute(name).set_value(it->second);
  else
  {
    ASSERT(false, ("Enum value not found in map for", name));
  }
}

/**
 * @brief Retrieves the IDs of replaced messages from an XML element.
 * @param node The XML element to retrieve (`merge`).
 * @param replacedIds Receives the replaced IDs.
 * @return `true` on success (including if the node contains no replaced IDs), `false` if the node does not exist or does not contain valid data.
 */
bool ReplacedMessageIdsFromXml(pugi::xml_node const & node, std::vector<std::string> & replacedIds)
{
  if (!node)
    return false;

  bool result = false;
  auto const replacedIdNodes = node.select_nodes("./replaces");

  if (replacedIdNodes.empty())
    return true;

  for (auto const & xpathNode : replacedIdNodes)
  {
    auto const replacedIdNode = xpathNode.node();
    std::string replacedId;
    if (StringFromXml(replacedIdNode.attribute("id"), replacedId))
    {
      replacedIds.push_back(replacedId);
      result = true;
    }
    else
      LOG(LWARNING, ("Could not parse merge element, skipping"));
  }
  return result;
}

/**
 * @brief Retrieves a latitude/longitude pair from an XML element.
 *
 * Coordinates must be given as latitude, followed by as space, then longitude. Latitude and
 * longitude are given as floating-point numbers, optionally with a sign (plus is assumed if no
 * sign is given). Coordinates are interpreted as degrees in WGS84 format.
 *
 * @param node The XML element to retrieve.
 * @param latLon Receives the latitude/longitude pair.
 * @return `true` on success, `false` if the node does not exist or does not contain valid coordinates.
 */
bool LatLonFromXml(pugi::xml_node const & node, ms::LatLon & latLon)
{
  if (!node)
    return false;
  std::string string;
  if (StringFromXml(node, string))
  {
    static boost::regex latLonRegex("([+-]?[0-9]*\\.?[0-9]*)\\s+([+-]?[0-9]*\\.?[0-9]*)");
    boost::smatch latLonMatcher;
    if (boost::regex_search(string, latLonMatcher, latLonRegex) && latLonMatcher[1].matched && latLonMatcher[2].matched)
    {
      try
      {
        latLon.m_lat = std::stod(latLonMatcher[1]);
        latLon.m_lon = std::stod(latLonMatcher[2]);
        return true;
      }
      catch (std::invalid_argument const& ex)
      {
        LOG(LWARNING, ("Not a valid coordinate pair:", string));
      }
    }
    else
      LOG(LWARNING, ("Not a valid coordinate pair:", string));
  }
  return false;
}

/**
 * @brief Retrieves a Traff `Point` from an XML element.
 * @param node The XML element to retrieve (any child of `location`).
 * @return The point, or `std::nullopt` if the node does not exist or does not contain valid point data,
 */
std::optional<Point> OptionalPointFromXml(pugi::xml_node const & node)
{
  if (!node)
    return std::nullopt;
  Point result;

  if (!LatLonFromXml(node, result.m_coordinates))
  {
    LOG(LWARNING, (node.name(), "has no coordinates, ignoring"));
    return std::nullopt;
  }

  result.m_junctionName = OptionalStringFromXml(node.attribute("junction_name"));
  result.m_junctionRef = OptionalStringFromXml(node.attribute("junction_ref"));
  result.m_distance = OptionalFloatFromXml(node.attribute("distance"));

  return result;
}

/**
 * @brief Adds a TraFF point to a node.
 *
 * @param point The TraFF point.
 * @param name The name of the node to store the TraFF point in.
 * @param parentNode The parent node to which the new node will be added (`location`).
 */
void PointToXml(Point const & point, std::string name, pugi::xml_node & parentNode)
{
  auto node = parentNode.append_child(name);
  if (point.m_distance)
    node.append_attribute("distance").set_value(point.m_distance.value());
  if (point.m_junctionName)
    node.append_attribute("junction_name").set_value(point.m_junctionName.value());
  if (point.m_junctionRef)
    node.append_attribute("junction_ref").set_value(point.m_junctionRef.value());

  std::ostringstream coord_ss;
  coord_ss << std::fixed << std::setprecision(5)
           << std::showpos << point.m_coordinates.m_lat << " " << point.m_coordinates.m_lon;
  node.text() = coord_ss.str().c_str();
}

/**
 * @brief Retrieves a `TraffLocation` from an XML element.
 * @param node The XML element to retrieve (`location`).
 * @param location Receives the location.
 * @return `true` on success, `false` if the node does not exist or does not contain valid location data.
 */
bool LocationFromXml(pugi::xml_node const & node, TraffLocation & location)
{
  if (!node)
    return false;

  location.m_from = OptionalPointFromXml(node.child("from"));
  location.m_to = OptionalPointFromXml(node.child("to"));
  location.m_at = OptionalPointFromXml(node.child("at"));
  location.m_via = OptionalPointFromXml(node.child("via"));
  location.m_notVia = OptionalPointFromXml(node.child("not_via"));

  int numPoints = 0;
  for (std::optional<Point> point : {location.m_from, location.m_to, location.m_at})
    if (point)
      numPoints++;
  // single-point locations are not supported, locations without points are not valid
  if (numPoints < 2)
  {
    LOG(LWARNING, ("Only", numPoints, "points of from/to/at specified, ignoring location"));
    return false;
  }

  location.m_country = OptionalStringFromXml(node.attribute("country"));
  location.m_destination = OptionalStringFromXml(node.attribute("destination"));
  location.m_direction = OptionalStringFromXml(node.attribute("direction"));

  EnumFromXml(node.attribute("directionality"), location.m_directionality, kDirectionalityMap);
  location.m_fuzziness = OptionalEnumFromXml(node.attribute("fuzziness"), kFuzzinessMap);
  location.m_origin = OptionalStringFromXml(node.attribute("origin"));
  EnumFromXml(node.attribute("ramps"), location.m_ramps, kRampsMap);
  location.m_roadClass = OptionalEnumFromXml(node.attribute("road_class"), kRoadClassMap);
  // disabled for now
  //location.m_roadIsUrban = OptionalBoolFromXml(node.attribute(("road_is_urban")));
  location.m_roadRef = OptionalStringFromXml(node.attribute("road_ref"));
  location.m_roadName = OptionalStringFromXml(node.attribute("road_name"));
  location.m_territory = OptionalStringFromXml(node.attribute("territory"));
  location.m_town = OptionalStringFromXml(node.attribute("town"));

  return true;
}

/**
 * @brief Stores a TraFF location in a node.
 *
 * @param location The TraFF location.
 * @param node The `location` node to store the location in.
 */
void LocationToXml(TraffLocation const & location, pugi::xml_node & node)
{
  if (location.m_country)
    node.append_attribute("country").set_value(location.m_country.value());
  if (location.m_destination)
    node.append_attribute("destination").set_value(location.m_destination.value());
  if (location.m_direction)
    node.append_attribute("direction").set_value(location.m_direction.value());
  EnumToXml(location.m_directionality, "directionality", node, kDirectionalityMap);
  if (location.m_fuzziness)
    EnumToXml(location.m_fuzziness.value(), "fuzziness", node, kFuzzinessMap);
  if (location.m_origin)
    node.append_attribute("origin").set_value(location.m_origin.value());
  EnumToXml(location.m_ramps, "ramps", node, kRampsMap);
  if (location.m_roadClass)
    EnumToXml(location.m_roadClass.value(), "road_class", node, kRoadClassMap);
  // TODO roadIsUrban (disabled for now)
  if (location.m_roadRef)
    node.append_attribute("road_ref").set_value(location.m_roadRef.value());
  if (location.m_roadName)
    node.append_attribute("roadName").set_value(location.m_roadName.value());
  if (location.m_territory)
    node.append_attribute("territory").set_value(location.m_territory.value());
  if (location.m_town)
    node.append_attribute("town").set_value(location.m_town.value());

  if (location.m_from)
    PointToXml(location.m_from.value(), "from", node);
  if (location.m_at)
    PointToXml(location.m_at.value(), "at", node);
  if (location.m_via)
    PointToXml(location.m_via.value(), "via", node);
  if (location.m_notVia)
    PointToXml(location.m_notVia.value(), "not_via", node);
  if (location.m_to)
    PointToXml(location.m_to.value(), "to", node);
}

/**
 * @brief Retrieves a `TraffQuantifier` from an XML element.
 *
 * The TraFF specification allows only one quantifier per event. The quantifier type depends on the
 * event type, and not all events allow quantifiers.
 *
 * Quantifiers which violate these constraints are not filtered out, i.e. this function may return a
 * quantifier for event types that do not allow quantifiers, or of a type illegal for the event type.
 * If an event contains multiple quantifiers of different types, any one of these quantifiers may be
 * returned, with no preference for legal quantifiers over illegal ones.
 *
 * @param node The node from which to retrieve the quantifier (`event`).
 * @return The quantifier, or `std::nullopt`
 */
std::optional<uint16_t> OptionalDurationFromXml(pugi::xml_attribute const & attribute)
{
  std::string durationString;
  if (!StringFromXml(attribute, durationString))
    return std::nullopt;

  /*
   * Valid time formats:
   * 01:30 (hh:mm)
   * 1 h
   * 30 min
   */
  static boost::regex durationRegex("(([0-9]+):([0-9]{2}))|(([0-9]+) *h)|(([0-9]+) *min)");
  boost::smatch durationMatcher;

  if (boost::regex_search(durationString, durationMatcher, durationRegex))
  {
    if (!durationMatcher.str(2).empty() && !durationMatcher.str(3).empty())
      return std::stoi(durationMatcher[2]) * 60 + std::stoi(durationMatcher[3]);
    else if (!durationMatcher.str(5).empty())
      return std::stoi(durationMatcher[5]) * 60;
    else if (!durationMatcher.str(7).empty())
      return std::stoi(durationMatcher[7]);
    UNREACHABLE();
    return std::nullopt;
  }
  else
  {
    LOG(LINFO, ("Not a valid duration:", durationString));
    return std::nullopt;
  }
}

/**
 * @brief Retrieves a `TraffEvent` from an XML element.
 * @param node The XML element to retrieve (`event`).
 * @param event Receives the event.
 * @return `true` on success, `false` if the node does not exist or does not contain valid event data.
 */
bool EventFromXml(pugi::xml_node const & node, TraffEvent & event)
{
  std::string eventClass;
  if (!StringFromXml(node.attribute("class"), eventClass))
  {
    LOG(LWARNING, ("No event class specified, ignoring"));
    return false;
  }
  if (!EnumFromXml(node.attribute("class"), event.m_class, kEventClassMap))
    return false;

  std::string eventType;
  if (!StringFromXml(node.attribute("type"), eventType))
  {
    LOG(LWARNING, ("No event type specified, ignoring"));
    return false;
  }
  if (!eventType.starts_with(eventClass + "_"))
  {
    LOG(LWARNING, ("Event type", eventType, "does not match event class", eventClass, "(ignoring)"));
    return false;
  }
  if (!EnumFromXml(node.attribute("type"), event.m_type, kEventTypeMap))
    return false;

  event.m_length = OptionalIntegerFromXml(node.attribute("length"));
  event.m_probability = OptionalIntegerFromXml(node.attribute("probability"));

  event.m_qDurationMins = OptionalDurationFromXml(node.attribute("q_duration"));

  // TODO other quantifiers (not yet implemented in struct)

  event.m_speed = OptionalIntegerFromXml(node.attribute("speed"));

  // TODO supplementary information (not yet implemented in struct)
  return true;
}

/**
 * @brief Stores a TraFF event in a node.
 *
 * @param event The TraFF event.
 * @param node The `event` node to store the event in.
 */
void EventToXml(TraffEvent const & event, pugi::xml_node & node)
{
  EnumToXml(event.m_class, "class", node, kEventClassMap);
  EnumToXml(event.m_type, "type", node, kEventTypeMap);
  if (event.m_length)
    node.append_attribute("length").set_value(event.m_length.value());
  if (event.m_probability)
    node.append_attribute("probability").set_value(event.m_probability.value());

  if (event.m_qDurationMins)
  {
    auto mins = event.m_qDurationMins.value();
    auto hours = mins / 60;
    auto remaining_mins = mins % 60;
    std::ostringstream duration_ss;
    duration_ss << std::setfill('0') << std::setw(2) << hours << ":" 
                << std::setw(2) << remaining_mins;
    node.append_attribute("q_duration").set_value(duration_ss.str().c_str());
  }

  // TODO other quantifiers (not yet implemented in struct)

  if (event.m_speed)
    node.append_attribute("speed").set_value(event.m_speed.value());

  // TODO supplementary information (not yet implemented in struct)
}

/**
 * @brief Retrieves the TraFF events associated with a message from an XML element.
 * @param node The XML element to retrieve (`events`).
 * @param events Receives the events.
 * @return `true` on success, `false` if the node does not exist or does not contain valid event data (including if the node contains no events).
 */
bool EventsFromXml(pugi::xml_node const & node, std::vector<TraffEvent> & events)
{
  if (!node)
    return false;

  bool result = false;
  auto const eventNodes = node.select_nodes("./event");

  if (eventNodes.empty())
    return false;

  for (auto const & xpathNode : eventNodes)
  {
    auto const eventNode = xpathNode.node();
    TraffEvent event;
    if (EventFromXml(eventNode, event))
    {
      events.push_back(event);
      result = true;
    }
    else
      LOG(LWARNING, ("Could not parse event, skipping"));
  }
  return result;
}

/**
 * @brief Retrieves a coloring segment (segment with speed group) from XML
 * @param node The `segment` node
 * @param coloring The coloring to which the segment will be added.
 * @return true if each segment was parsed successfully, false if errors occurred (in this case,
 * the decoded coloring for this message should be discarded and regenerated from scratch)
 */
bool SegmentFromXml(pugi::xml_node const & node,
               std::map<traffic::TrafficInfo::RoadSegmentId, traffic::SpeedGroup> & coloring)
{
  uint32_t fid;
  // initialize to get rid of compiler warnings (which are false alerts)
  uint16_t idx = 0;
  uint8_t dir = 0;
  if (IntegerFromXml(node.attribute("fid"), fid)
      && IntegerFromXml(node.attribute("idx"), idx)
      && IntegerFromXml(node.attribute("dir"), dir))
  {
    traffic::TrafficInfo::RoadSegmentId segment(fid, idx, dir);
    traffic::SpeedGroup sg = traffic::SpeedGroup::Unknown;
    if (EnumFromXml(node.attribute("speed_group"), sg, kSpeedGroupMap))
      coloring[segment] = sg;
    else
    {
      LOG(LWARNING, ("missing or invalid speed group for", segment, "(aborting)"));
      return false;
    }
  }
  else
  {
    LOG(LWARNING, ("segment with incomplete information (fid, idx, dir), aborting"));
    return false;
  }
  return true;
}

/**
 * @brief Retrieves coloring for a single MWM from XML.
 *
 * This function returns false if errors occurred during decoding (due to invalid data), or if the
 * data version to which the segments refer does not coincide with the currently used version of the
 * corresponding MWM. In this case, the entire coloring for this message should be discarded and the
 * message should be decoded from scratch.
 *
 * @todo Errors in segments are currently not considered, i.e. this function may return true even if
 * one or more segments have errors.
 *
 * @param node The `coloring` node.
 * @param dataSource The data source for coloring.
 * @param decoded Receives the decoded global coloring.
 * @return whether the decoded segments can be used, see description
 */
bool ColoringFromXml(pugi::xml_node const & node, DataSource const & dataSource,
                     MultiMwmColoring & decoded)
{
  std::string countryName;
  if (!StringFromXml(node.attribute("country_name"), countryName))
  {
    LOG(LWARNING, ("coloring element without country_name attribute, skipping"));
    return false;
  }
  auto const & mwmId = dataSource.GetMwmIdByCountryFile(platform::CountryFile(countryName));
  if (!mwmId.IsAlive())
  {
    LOG(LWARNING, ("Can’t get MWM ID for country", countryName, "(skipping)"));
    return false;
  }

  int64_t version = 0;
  if (!IntegerFromXml(node.attribute("version"), version))
  {
    LOG(LWARNING, ("Can’t get version for country", countryName, "(skipping)"));
    return false;
  }
  else if (version != mwmId.GetInfo()->GetVersion())
  {
    LOG(LINFO, ("XML data for country", countryName, "has version", version, "while MWM has", mwmId.GetInfo()->GetVersion(), "(skipping)"));
    return false;
  }

  auto const segmentNodes = node.select_nodes("./segment");

  if (segmentNodes.empty())
    return true;

  std::map<traffic::TrafficInfo::RoadSegmentId, traffic::SpeedGroup> coloring;

  for (auto const & segmentXpathNode : segmentNodes)
  {
    auto const & segmentNode = segmentXpathNode.node();
    if (!SegmentFromXml(segmentNode, coloring))
      return false;
  }

  if (!coloring.empty())
    decoded[mwmId] = coloring;

  return true;
}

/**
 * @brief Stores coloring for an indidual MWM in an XML node.
 *
 * The vaues of `mwmId` will be added to `node` as attributes. The segments and their traffic group
 * will be added to `node` as child nodes.
 *
 * @param mwmId
 * @param coloring
 * @param node The `coloring` node to store the coloring in.
 */
void ColoringToXml(MwmSet::MwmId const & mwmId,
                   std::map<traffic::TrafficInfo::RoadSegmentId, traffic::SpeedGroup> const & coloring,
                   pugi::xml_node node)
{
  node.append_attribute("country_name").set_value(mwmId.GetInfo()->GetCountryName());
  node.append_attribute("version").set_value(mwmId.GetInfo()->GetVersion());
  for (auto & [segId, sg] : coloring)
  {
    auto segNode = node.append_child("segment");
    segNode.append_attribute("fid").set_value(segId.GetFid());
    segNode.append_attribute("idx").set_value(segId.GetIdx());
    segNode.append_attribute("dir").set_value(segId.GetDir());
    EnumToXml(sg, "speed_group", segNode, kSpeedGroupMap);
  }
}

/**
 * @brief Retrieves global coloring from XML.
 *
 * If the MWM version does not match for at least one MWM, no coloring is decoded (`decoded` is
 * empty after this function returns) and the message needs to be decoded from scratch.
 *
 * @param node The `mwm_coloring` node.
 * @param dataSource The data source for coloring, see `ParseTraff()`.
 * @param decoded Receives the decoded global coloring.
 */
void AllMwmColoringFromXml(pugi::xml_node const & node,
                           std::optional<std::reference_wrapper<const DataSource>> dataSource,
                           MultiMwmColoring & decoded)
{
  if (!node)
    return;

  if (!dataSource)
  {
    LOG(LWARNING, ("Message has mwm_coloring but it cannot be parsed as no data source was specified"));
    return;
  }

  auto const coloringNodes = node.select_nodes("./coloring");

  if (coloringNodes.empty())
    return;

  for (auto const & coloringXpathNode : coloringNodes)
  {
    auto const & coloringNode = coloringXpathNode.node();
    if (!ColoringFromXml(coloringNode, dataSource->get(), decoded))
    {
      decoded.clear();
      return;
    }
  }
}

/**
 * @brief Retrieves a TraFF message from an XML element.
 * @param node The XML element to retrieve (`message`).
 * @param dataSource The data source for coloring, see `ParseTraff()`.
 * @param message Receives the message.
 * @return `true` on success, `false` if the node does not exist or does not contain valid message data.
 */
bool MessageFromXml(pugi::xml_node const & node,
                    std::optional<std::reference_wrapper<const DataSource>> dataSource,
                    TraffMessage & message)
{
  if (!StringFromXml(node.attribute("id"), message.m_id))
  {
    LOG(LWARNING, ("Message has no id"));
    return false;
  }

  if (!TimeFromXml(node.attribute("receive_time"), message.m_receiveTime))
  {
    LOG(LWARNING, ("Message", message.m_id, "has no receive_time"));
    return false;
  }

  if (!TimeFromXml(node.attribute("update_time"), message.m_updateTime))
  {
    LOG(LWARNING, ("Message", message.m_id, "has no update_time"));
    return false;
  }

  if (!TimeFromXml(node.attribute("expiration_time"), message.m_expirationTime))
  {
    LOG(LWARNING, ("Message", message.m_id, "has no expiration_time"));
    return false;
  }

  message.m_startTime = OptionalTimeFromXml(node.attribute("start_time"));
  message.m_endTime = OptionalTimeFromXml(node.attribute("end_time"));

  message.m_cancellation = BoolFromXml(node.attribute("cancellation"), false);
  message.m_forecast = BoolFromXml(node.attribute("forecast"), false);

  // TODO urgency (not yet implemented in struct)

  ReplacedMessageIdsFromXml(node.child("merge"), message.m_replaces);

  if (!message.m_cancellation)
  {
    message.m_location.emplace();
    if (LocationFromXml(node.child("location"), message.m_location.value()))
      AllMwmColoringFromXml(node.child("mwm_coloring"), dataSource, message.m_decoded);
    else
    {
      message.m_location.reset();
      LOG(LWARNING, ("Message", message.m_id, "has no location but is not a cancellation message"));
      return false;
    }

    if (!EventsFromXml(node.child("events"), message.m_events))
      {
        LOG(LWARNING, ("Message", message.m_id, "has no events but is not a cancellation message"));
        return false;
      }
  }
  return true;
}

/**
 * @brief Stores a TraFF message in a node.
 *
 * @param message The TraFF message.
 * @param node The `message` node to store the message in.
 */
void MessageToXml(TraffMessage const & message, pugi::xml_node node)
{
  node.append_attribute("id").set_value(message.m_id);
  node.append_attribute("receive_time").set_value(message.m_receiveTime.ToString());
  node.append_attribute("update_time").set_value(message.m_updateTime.ToString());
  node.append_attribute("expiration_time").set_value(message.m_expirationTime.ToString());
  if (message.m_startTime)
    node.append_attribute("start_time").set_value(message.m_startTime.value().ToString());
  if (message.m_endTime)
    node.append_attribute("end_time").set_value(message.m_endTime.value().ToString());

  node.append_attribute("cancellation").set_value(message.m_cancellation);
  node.append_attribute("forecast").set_value(message.m_forecast);

  // TODO urgency (not yet implemented in struct)

  if (!message.m_replaces.empty())
  {
    auto mergeNode = node.append_child("merge");
    for (auto const & id : message.m_replaces)
    {
      auto replacesNode = mergeNode.append_child("replaces");
      replacesNode.append_attribute("id").set_value(id);
    }
  }

  if (message.m_location)
  {
    auto locationNode = node.append_child("location");
    LocationToXml(message.m_location.value(), locationNode);
  }

  if (!message.m_events.empty())
  {
    auto eventsNode = node.append_child("events");
    for (auto const event : message.m_events)
    {
      auto eventNode = eventsNode.append_child("event");
      EventToXml(event, eventNode);
    }
  }

  if (!message.m_decoded.empty())
  {
    auto allMwmColoringNode = node.append_child("mwm_coloring");
    for (auto & [mwmId, coloring] : message.m_decoded)
    {
      auto coloringNode = allMwmColoringNode.append_child("coloring");
      ColoringToXml(mwmId, coloring, coloringNode);
    }
  }
}

/**
 * @brief Retrieves a TraFF feed from an XML element.
 * @param node The XML element to retrieve (`feed`).
 * @param dataSource The data source for coloring, see `ParseTraff()`.
 * @param feed Receives the feed.
 * @return `true` on success, `false` if the node does not exist or does not contain valid message data.
 */
bool FeedFromXml(pugi::xml_node const & node,
                    std::optional<std::reference_wrapper<const DataSource>> dataSource,
                    TraffFeed & feed)
{
  bool result = false;

  // Select all messages elements that are direct children of the node.
  auto const messages = node.select_nodes("./message");

  if (messages.empty())
    return true;

  // TODO try block?
  for (auto const & xpathNode : messages)
  {
    auto const messageNode = xpathNode.node();
    TraffMessage message;
    if (MessageFromXml(messageNode, dataSource, message))
    {
      feed.push_back(message);
      result = true;
    }
    else
      LOG(LWARNING, ("Could not parse message, skipping"));
  }
  return result;
}

bool ParseTraff(pugi::xml_document const & document,
                std::optional<std::reference_wrapper<const DataSource>> dataSource,
                TraffFeed & feed)
{
  return FeedFromXml(document.document_element(), dataSource, feed);
}

void GenerateTraff(TraffFeed const & feed, pugi::xml_document & document)
{
  auto root = document.append_child("feed");
  for (auto const & message : feed)
  {
    auto child = root.append_child("message");
    MessageToXml(message, child);
  }
}

void GenerateTraff(std::map<std::string, traffxml::TraffMessage> const & messages,
                   pugi::xml_document & document)
{
  auto root = document.append_child("feed");
  for (auto const & [id, message] : messages)
  {
    auto child = root.append_child("message");
    MessageToXml(message, child);
  }
}

std::string FiltersToXml(std::vector<m2::RectD> & bboxRects)
{
  std::ostringstream os;
  for (auto rect : bboxRects)
    os << "<filter bbox=\"" << mercator::YToLat(rect.minY()) << " "
       << mercator::XToLon(rect.minX()) << " "
       << mercator::YToLat(rect.maxY()) << " "
       << mercator::XToLon(rect.maxX()) << "\"/>\n";
  return os.str();
}

TraffResponse ParseResponse(std::string const & responseXml)
{
  TraffResponse result;
  pugi::xml_document responseDocument;
  if (!responseDocument.load_string(responseXml.c_str()))
    return result;

  auto const responseElement = responseDocument.document_element();
  std::string responseElementName(responseElement.name());

  if (responseElementName != "response")
    return result;

  if (!ResponseStatusFromXml(responseElement.attribute("status"), result.m_status))
    return result;

  StringFromXml(responseElement.attribute("subscription_id"), result.m_subscriptionId);

  IntegerFromXml(responseElement.attribute("timeout"), result.m_timeout);

  LOG(LDEBUG, ("Response, status:", result.m_status, "subscription ID:", result.m_subscriptionId, "timeout:", result.m_timeout));

  if (responseElement.child("feed"))
  {
    TraffFeed feed;
    FeedFromXml(responseElement.child("feed"), std::nullopt /* dataSource */, feed);
    LOG(LDEBUG, ("Feed received, number of messages:", feed.size()));
    result.m_feed = std::move(feed);
  }
  else
    LOG(LDEBUG, ("No feed in response"));

  return result;
}
}  // namespace openlr
