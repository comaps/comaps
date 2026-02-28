#include "indexer/map_style.hpp"

#include "base/assert.hpp"

MapStyle const kDefaultMapStyle = MapStyleWalkingLight;

MapStyle MapStyleFromSettings(std::string const & str)
{
  // MapStyleMerged is service style. It's unavailable for users.
  if (str == "MapStyleWalkingLight")
    return MapStyleWalkingLight;
  else if (str == "MapStyleWalkingOutdoorLight")
    return MapStyleWalkingOutdoorLight;
  else if (str == "MapStyleWalkingDark")
    return MapStyleWalkingDark;
  else if (str == "MapStyleWalkingOutdoorDark")
    return MapStyleWalkingOutdoorDark;
  else if (str == "MapStyleCyclingLight")
    return MapStyleCyclingLight;
  else if (str == "MapStyleCyclingOutdoorLight")
    return MapStyleCyclingOutdoorLight;
  else if (str == "MapStyleCyclingDark")
    return MapStyleCyclingDark;
  else if (str == "MapStyleCyclingOutdoorDark")
    return MapStyleCyclingOutdoorDark;
  else if (str == "MapStyleDrivingLight")
    return MapStyleDrivingLight;
  else if (str == "MapStyleDrivingOutdoorLight")
    return MapStyleDrivingOutdoorLight;
  else if (str == "MapStyleDrivingDark")
    return MapStyleDrivingDark;
  else if (str == "MapStyleDrivingOutdoorDark")
    return MapStyleDrivingOutdoorDark;
  else if (str == "MapStylePublicTransportLight")
    return MapStylePublicTransportLight;
  else if (str == "MapStylePublicTransportOutdoorLight")
    return MapStylePublicTransportOutdoorLight;
  else if (str == "MapStylePublicTransportDark")
    return MapStylePublicTransportDark;
  else if (str == "MapStylePublicTransportOutdoorDark")
    return MapStylePublicTransportOutdoorDark;
  else if (str == "MapStyleVehicleLight")
    return MapStyleVehicleLight;
  else if (str == "MapStyleVehicleDark")
    return MapStyleVehicleDark;

  return kDefaultMapStyle;
}

std::string MapStyleToString(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleWalkingLight: return "MapStyleWalkingLight";
  case MapStyleWalkingOutdoorLight: return "MapStyleWalkingOutdoorLight";
  case MapStyleWalkingDark: return "MapStyleWalkingDark";
  case MapStyleWalkingOutdoorDark: return "MapStyleWalkingOutdoorDark";
  case MapStyleCyclingLight: return "MapStyleCyclingLight";
  case MapStyleCyclingOutdoorLight: return "MapStyleCyclingOutdoorLight";
  case MapStyleCyclingDark: return "MapStyleCyclingDark";
  case MapStyleCyclingOutdoorDark: return "MapStyleCyclingOutdoorDark";
  case MapStyleDrivingLight: return "MapStyleDrivingLight";
  case MapStyleDrivingOutdoorLight: return "MapStyleDrivingOutdoorLight";
  case MapStyleDrivingDark: return "MapStyleDrivingDark";
  case MapStyleDrivingOutdoorDark: return "MapStyleDrivingOutdoorDark";
  case MapStylePublicTransportLight: return "MapStylePublicTransportLight";
  case MapStylePublicTransportOutdoorLight: return "MapStylePublicTransportOutdoorLight";
  case MapStylePublicTransportDark: return "MapStylePublicTransportDark";
  case MapStylePublicTransportOutdoorDark: return "MapStylePublicTransportOutdoorDark";
  case MapStyleMerged: return "MapStyleMerged";
  case MapStyleVehicleLight: return "MapStyleVehicleLight";
  case MapStyleVehicleDark: return "MapStyleVehicleDark";

  case MapStyleCount: break;
  }
  ASSERT(false, ());
  return std::string();
}

std::string DebugPrint(MapStyle mapStyle)
{
  return MapStyleToString(mapStyle);
}

MapStyle GetLightMapStyleVariant(MapStyle mapStyle)
{
  if (!MapStyleIsDark(mapStyle))
    return mapStyle;

  switch (mapStyle)
  {
  case MapStyleWalkingDark: return MapStyleWalkingLight;
  case MapStyleWalkingOutdoorDark: return MapStyleWalkingOutdoorLight;
  case MapStyleCyclingDark: return MapStyleCyclingLight;
  case MapStyleCyclingOutdoorDark: return MapStyleCyclingOutdoorLight;
  case MapStyleDrivingDark: return MapStyleDrivingLight;
  case MapStyleDrivingOutdoorDark: return MapStyleDrivingOutdoorLight;
  case MapStylePublicTransportDark: return MapStylePublicTransportLight;
  case MapStylePublicTransportOutdoorDark: return MapStylePublicTransportOutdoorLight;
  case MapStyleVehicleDark: return MapStyleVehicleLight;
  default: CHECK(false, ()); return MapStyleWalkingLight;
  }
}

MapStyle GetDarkMapStyleVariant(MapStyle mapStyle)
{
  if (MapStyleIsDark(mapStyle) || mapStyle == MapStyleMerged)
    return mapStyle;

  switch (mapStyle)
  {
  case MapStyleWalkingLight: return MapStyleWalkingDark;
  case MapStyleWalkingOutdoorLight: return MapStyleWalkingOutdoorDark;
  case MapStyleCyclingLight: return MapStyleCyclingDark;
  case MapStyleCyclingOutdoorLight: return MapStyleCyclingOutdoorDark;
  case MapStyleDrivingLight: return MapStyleDrivingDark;
  case MapStyleDrivingOutdoorLight: return MapStyleDrivingOutdoorDark;
  case MapStylePublicTransportLight: return MapStylePublicTransportDark;
  case MapStylePublicTransportOutdoorLight: return MapStylePublicTransportOutdoorDark;
  case MapStyleVehicleLight: return MapStyleVehicleDark;
  default: CHECK(false, ()); return MapStyleWalkingDark;
  }
}

bool MapStyleIsDark(MapStyle mapStyle)
{
  for (auto const darkStyle : {MapStyleWalkingDark, MapStyleWalkingOutdoorDark, MapStyleCyclingDark, MapStyleCyclingOutdoorDark, MapStyleDrivingDark, MapStyleDrivingOutdoorDark, MapStylePublicTransportDark, MapStylePublicTransportOutdoorDark, MapStyleVehicleDark})
    if (mapStyle == darkStyle)
      return true;
  return false;
}

MapStyle GetRegularMapStyleVariant(MapStyle mapStyle)
{
  if (!MapStyleIsOutdoor(mapStyle))
    return mapStyle;

  switch (mapStyle)
  {
  case MapStyleWalkingOutdoorLight: return MapStyleWalkingLight;
  case MapStyleWalkingOutdoorDark: return MapStyleWalkingDark;
  case MapStyleCyclingOutdoorLight: return MapStyleCyclingLight;
  case MapStyleCyclingOutdoorDark: return MapStyleCyclingDark;
  case MapStyleDrivingOutdoorLight: return MapStyleDrivingLight;
  case MapStyleDrivingOutdoorDark: return MapStyleDrivingDark;
  case MapStylePublicTransportOutdoorLight: return MapStylePublicTransportLight;
  case MapStylePublicTransportOutdoorDark: return MapStylePublicTransportDark;
  case MapStyleVehicleLight: return MapStyleVehicleLight;
  case MapStyleVehicleDark: return MapStyleVehicleLight;
  default: CHECK(false, ()); return MapStyleWalkingLight;
  }
}

MapStyle GetOutdoorMapStyleVariant(MapStyle mapStyle)
{
  if (MapStyleIsOutdoor(mapStyle) || mapStyle == MapStyleMerged)
    return mapStyle;

  switch (mapStyle)
  {
  case MapStyleWalkingLight: return MapStyleWalkingOutdoorLight;
  case MapStyleWalkingDark: return MapStyleWalkingOutdoorDark;
  case MapStyleCyclingLight: return MapStyleCyclingOutdoorLight;
  case MapStyleCyclingDark: return MapStyleCyclingOutdoorDark;
  case MapStyleDrivingLight: return MapStyleDrivingOutdoorLight;
  case MapStyleDrivingDark: return MapStyleDrivingOutdoorDark;
  case MapStylePublicTransportLight: return MapStylePublicTransportOutdoorLight;
  case MapStylePublicTransportDark: return MapStylePublicTransportOutdoorDark;
  case MapStyleVehicleLight: return MapStyleVehicleLight;
  case MapStyleVehicleDark: return MapStyleVehicleDark;
  default: CHECK(false, ()); return MapStyleWalkingOutdoorLight;
  }
}

bool MapStyleIsOutdoor(MapStyle mapStyle)
{
  for (auto const outdoorStyle : {MapStyleWalkingOutdoorLight, MapStyleWalkingOutdoorDark, MapStyleCyclingOutdoorLight, MapStyleCyclingOutdoorDark, MapStyleDrivingOutdoorLight, MapStyleDrivingOutdoorDark, MapStylePublicTransportOutdoorLight, MapStylePublicTransportOutdoorDark})
    if (mapStyle == outdoorStyle)
      return true;
  return false;
}
