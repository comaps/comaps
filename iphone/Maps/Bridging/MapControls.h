#import <Foundation/Foundation.h>

NS_SWIFT_NAME(MapControls)
@interface MapControls : NSObject

+ (MapControls *)shared;

- (bool)hasMainButtons;

+ (NSInteger)modeRawValue;
+ (void)setModeRawValue:(NSInteger)modeRawValue;
    
+ (BOOL)drivingModeHasTraffic;
+ (void)drivingModeSetTraffic:(BOOL)hasTraffic;

+ (BOOL)publicTransportModeHasTransitLines;
+ (void)publicTransportModeSetTransitLines:(BOOL)hasTransitLines;

+ (BOOL)outdoorLayerEnabled;
+ (void)setOutdoorLayerEnabled:(BOOL)outdoorLayerEnabled;

+ (BOOL)contourLinesLayerEnabled;
+ (void)setContourLinesLayerEnabled:(BOOL)contourLinesLayerEnabled;

+ (BOOL)buildings3dLayerEnabled;
+ (void)setBuildings3dLayerEnabled:(BOOL)buildings3dLayerEnabled;

+ (void)zoomScale:(CGFloat)scale;
+ (void)zoomIn;
+ (void)zoomOut;

+ (void)switchToNextPositionMode;
+ (NSString *)positionModeRawValue;

@end
