#import "SwiftBridge.h"
#import "MapControls.h"

#include <CoreApi/Framework.h>

@implementation MapControls

static MapControls *shared = nil;

+ (MapControls *)shared
{
  if (shared == nil)
    shared = [[super allocWithZone:NULL] init];
  
  return shared;
}

+ (id)allocWithZone:(NSZone *)zone
{
  return [self shared];
}

- (id)copyWithZone:(NSZone *)zone
{
  return self;
}

- (id)init
{
  self = [super init];
  return self;
}

- (bool)hasMainButtons;
{
  return true;
}

+ (NSInteger)modeRawValue {
  return GetFramework().CurrentMapMode();
}

+ (void)setModeRawValue:(NSInteger)modeRawValue {
  GetFramework().SwitchToMapMode(static_cast<MapMode>(modeRawValue));
}

+ (BOOL)drivingModeHasTraffic {
  return GetFramework().DrivingMapModeHasTraffic();
}

+ (void)drivingModeSetTraffic:(BOOL)hasTraffic {
  GetFramework().DrivingMapModeSetTraffic(hasTraffic);
}

+ (BOOL)publicTransportModeHasTransitLines {
  return GetFramework().PublicTransportMapModeHasTransitLines();
}

+ (void)publicTransportModeSetTransitLines:(BOOL)hasTransitLines {
  GetFramework().PublicTransportMapModeSetTransitLines(hasTransitLines);
}

+ (BOOL)outdoorLayerEnabled
{
  return GetFramework().HasOutdoorLayer();
}

+ (void)setOutdoorLayerEnabled:(BOOL)outdoorLayerEnabled
{
  GetFramework().SetOutdoorLayer(outdoorLayerEnabled);
}

+ (BOOL)contourLinesLayerEnabled
{
  return GetFramework().HasContourLinesLayer();
}

+ (void)setContourLinesLayerEnabled:(BOOL)contourLinesLayerEnabled
{
  GetFramework().SetContourLinesLayer(contourLinesLayerEnabled);
}

+ (BOOL)buildings3dLayerEnabled;
{
  return GetFramework().HasBuildings3d();
}

+ (void)setBuildings3dLayerEnabled:(BOOL)buildings3dLayerEnabled;
{
  GetFramework().SetBuildings3d(buildings3dLayerEnabled);
}

+ (void)zoomScale:(CGFloat)scale;
{
  GetFramework().Scale(scale, true);
}

+ (void)zoomIn;
{
  GetFramework().Scale(Framework::SCALE_MAG, true);
}

+ (void)zoomOut;
{
  GetFramework().Scale(Framework::SCALE_MIN, true);
}

+ (NSString *)positionModeRawValue;
{
  location::EMyPositionMode mode = GetFramework().GetMyPositionMode();
  switch (mode)
  {
    case location::EMyPositionMode::NotFollowNoPosition: return @"Locate";
    case location::EMyPositionMode::NotFollow: return @"Locate";
    case location::EMyPositionMode::PendingPosition: return @"Locating";
    case location::EMyPositionMode::Follow: return @"Following";
    case location::EMyPositionMode::FollowAndRotate: return @"FollowingAndRotated";
  }
  return @"Locate";
}

+ (void)switchToNextPositionMode;
{
  [MWMLocationManager enableLocationAlert];
  GetFramework().SwitchMyPositionNextMode();
}

@end
