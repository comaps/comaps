#pragma once

#include <string>

enum MapStyle
{
  MapStyleWalkingLight = 0,
  MapStyleWalkingOutdoorLight = 1,
  MapStyleWalkingDark = 2,
  MapStyleWalkingOutdoorDark = 3,
  MapStyleCyclingLight = 4,
  MapStyleCyclingOutdoorLight = 5,
  MapStyleCyclingDark = 6,
  MapStyleCyclingOutdoorDark = 7,
  MapStyleDrivingLight = 8,
  MapStyleDrivingOutdoorLight = 9,
  MapStyleDrivingDark = 10,
  MapStyleDrivingOutdoorDark = 11,
  MapStylePublicTransportLight = 12,
  MapStylePublicTransportOutdoorLight = 13,
  MapStylePublicTransportDark = 14,
  MapStylePublicTransportOutdoorDark = 15,
  MapStyleVehicleLight = 16,
  MapStyleVehicleDark = 17,
  MapStyleMerged = 18,
  // Add new map style here

  // Specifies number of MapStyle enum values, must be last
  MapStyleCount
};

extern MapStyle const kDefaultMapStyle;

extern MapStyle MapStyleFromSettings(std::string const & str);
extern std::string MapStyleToString(MapStyle mapStyle);
extern std::string DebugPrint(MapStyle mapStyle);
extern MapStyle GetLightMapStyleVariant(MapStyle mapStyle);
extern MapStyle GetDarkMapStyleVariant(MapStyle mapStyle);
extern bool MapStyleIsDark(MapStyle mapStyle);
extern MapStyle GetRegularMapStyleVariant(MapStyle mapStyle);
extern MapStyle GetOutdoorMapStyleVariant(MapStyle mapStyle);
extern bool MapStyleIsOutdoor(MapStyle mapStyle);
