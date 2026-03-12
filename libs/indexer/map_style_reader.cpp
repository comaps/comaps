#include "map_style_reader.hpp"

#include "indexer/map_style.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "indexer/classificator_loader.hpp"

namespace
{
std::string const kSuffixLight = "light";
std::string const kSuffixDark = "dark";
std::string const kSuffixWalkingLight = "_walking_light";
std::string const kSuffixWalkingOutdoorLight = "_walking_outdoor_light";
std::string const kSuffixWalkingDark = "_walking_dark";
std::string const kSuffixWalkingOutdoorDark  = "_walking_outdoor_dark";
std::string const kSuffixCyclingLight = "_cycling_light";
std::string const kSuffixCyclingOutdoorLight  = "_cycling_outdoor_light";
std::string const kSuffixCyclingDark = "_cycling_dark";
std::string const kSuffixCyclingOutdoorDark  = "_cycling_outdoor_dark";
std::string const kSuffixDrivingLight = "_driving_light";
std::string const kSuffixDrivingOutdoorLight  = "_driving_outdoor_light";
std::string const kSuffixDrivingDark = "_driving_dark";
std::string const kSuffixDrivingOutdoorDark  = "_driving_outdoor_dark";
std::string const kSuffixPublicTransportLight = "_public-transport_light";
std::string const kSuffixPublicTransportOutdoorLight  = "_public-transport_outdoor_light";
std::string const kSuffixPublicTransportDark = "_public-transport_dark";
std::string const kSuffixPublicTransportOutdoorDark  = "_public-transport_outdoor_dark";
std::string const kSuffixVehicleLight = "_vehicle_light";
std::string const kSuffixVehicleDark = "_vehicle_dark";

std::string const kStylesOverrideDir = "styles";

#ifdef BUILD_DESIGNER
std::string const kSuffixDesignTool = "design";
#endif  // BUILD_DESIGNER

std::string GetStyleRulesSuffix(MapStyle mapStyle)
{
#ifdef BUILD_DESIGNER
  return "_" + kSuffixDesignTool;
#else
  switch (mapStyle)
  {
  case MapStyleWalkingLight: return kSuffixWalkingLight;
  case MapStyleWalkingOutdoorLight: return kSuffixWalkingOutdoorLight;
  case MapStyleWalkingDark: return kSuffixWalkingDark;
  case MapStyleWalkingOutdoorDark: return kSuffixWalkingOutdoorDark;
  case MapStyleCyclingLight: return kSuffixCyclingLight;
  case MapStyleCyclingOutdoorLight: return kSuffixCyclingOutdoorLight;
  case MapStyleCyclingDark: return kSuffixCyclingDark;
  case MapStyleCyclingOutdoorDark: return kSuffixCyclingOutdoorDark;
  case MapStyleDrivingLight: return kSuffixDrivingLight;
  case MapStyleDrivingOutdoorLight: return kSuffixDrivingOutdoorLight;
  case MapStyleDrivingDark: return kSuffixDrivingDark;
  case MapStyleDrivingOutdoorDark: return kSuffixDrivingOutdoorDark;
  case MapStylePublicTransportLight: return kSuffixPublicTransportLight;
  case MapStylePublicTransportOutdoorLight: return kSuffixPublicTransportOutdoorLight;
  case MapStylePublicTransportDark: return kSuffixPublicTransportDark;
  case MapStylePublicTransportOutdoorDark: return kSuffixPublicTransportOutdoorDark;
  case MapStyleVehicleLight: return kSuffixVehicleLight;
  case MapStyleVehicleDark: return kSuffixVehicleDark;
  case MapStyleMerged: return {};

  case MapStyleCount: break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixWalkingLight;
#endif  // BUILD_DESIGNER
}

std::string GetStyleResourcesSuffix(MapStyle mapStyle)
{
#ifdef BUILD_DESIGNER
  return kSuffixDesignTool;
#else
  // We use the same resources for all light/day and dark/night styles
  // to avoid textures duplication and package size increasing.
  switch (mapStyle)
  {
  case MapStyleWalkingLight:
  case MapStyleWalkingOutdoorLight:
  case MapStyleCyclingLight:
  case MapStyleCyclingOutdoorLight:
  case MapStyleDrivingLight:
  case MapStyleDrivingOutdoorLight:
  case MapStylePublicTransportLight:
  case MapStylePublicTransportOutdoorLight:
  case MapStyleVehicleLight: return kSuffixLight;
  case MapStyleWalkingDark:
  case MapStyleWalkingOutdoorDark:
  case MapStyleCyclingDark:
  case MapStyleCyclingOutdoorDark:
  case MapStyleDrivingDark:
  case MapStyleDrivingOutdoorDark:
  case MapStylePublicTransportDark:
  case MapStylePublicTransportOutdoorDark:
  case MapStyleVehicleDark: return kSuffixDark;
  case MapStyleMerged: return {};

  case MapStyleCount: break;
  }
  LOG(LWARNING, ("Unknown map style", mapStyle));
  return kSuffixLight;
#endif  // BUILD_DESIGNER
}
}  // namespace

StyleReader::StyleReader() : m_mapStyle(kDefaultMapStyle) {}

void StyleReader::SetCurrentStyle(MapStyle mapStyle)
{
  m_loadingMapStyle = mapStyle;
  classificator::Load();
  m_mapStyle = mapStyle;
  classificator::Cleanup();
}

MapStyle StyleReader::GetCurrentStyle() const
{
  return m_mapStyle;
}

MapStyle StyleReader::GetLoadingStyle() const
{
  return m_loadingMapStyle;
}

bool StyleReader::IsCarNavigationStyle() const
{
  return m_mapStyle == MapStyle::MapStyleVehicleLight || m_mapStyle == MapStyle::MapStyleVehicleDark;
}

ReaderPtr<Reader> StyleReader::GetDrawingRulesReader() const
{
  std::string rulesFile = std::string("drules_proto") + GetStyleRulesSuffix(GetLoadingStyle()) + ".bin";

  auto overriddenRulesFile = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, rulesFile);
  if (Platform::IsFileExistsByFullPath(overriddenRulesFile))
    rulesFile = overriddenRulesFile;

#ifdef BUILD_DESIGNER
  // For Designer tool we have to look first into the resource folder.
  return GetPlatform().GetReader(rulesFile, "rwf");
#else
  return GetPlatform().GetReader(rulesFile);
#endif
}

ReaderPtr<Reader> StyleReader::GetResourceReader(std::string const & file, std::string_view density) const
{
  std::string resFile =
      base::JoinPath("symbols", std::string{density}, GetStyleResourcesSuffix(GetCurrentStyle()), file);

  auto overriddenResFile = base::JoinPath(GetPlatform().WritableDir(), kStylesOverrideDir, resFile);
  if (GetPlatform().IsFileExistsByFullPath(overriddenResFile))
    resFile = overriddenResFile;

#ifdef BUILD_DESIGNER
  // For Designer tool we have to look first into the resource folder.
  return GetPlatform().GetReader(resFile, "rwf");
#else
  return GetPlatform().GetReader(resFile);
#endif
}

ReaderPtr<Reader> StyleReader::GetDefaultResourceReader(std::string const & file) const
{
  return GetPlatform().GetReader(base::JoinPath("symbols/walking", file));
}

StyleReader & GetStyleReader()
{
  static StyleReader instance;
  return instance;
}
