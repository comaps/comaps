#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, MWMBorderAvoidanceMode) {
  MWMBorderAvoidanceModeNone = 0,
  MWMBorderAvoidanceModeAny = 1,
  MWMBorderAvoidanceModeNonInternal = 2,
  MWMBorderAvoidanceModeSpecific = 3,
};

NS_SWIFT_NAME(RoutingOptions)
@interface MWMRoutingOptions : NSObject

@property(nonatomic) BOOL avoidToll;
@property(nonatomic) BOOL avoidDirty;
@property(nonatomic) BOOL avoidPaved;
@property(nonatomic) BOOL avoidFerry;
@property(nonatomic) BOOL avoidMotorway;
@property(nonatomic) BOOL avoidSteps;
@property(nonatomic, readonly) BOOL hasOptions;

@property(nonatomic) MWMBorderAvoidanceMode borderAvoidanceMode;
@property(nonatomic, copy) NSSet<NSString *> * avoidedBorderCountries;
@property(nonatomic, readonly) NSArray<NSString *> * topLevelCountries;

- (void)save;

@end

NS_ASSUME_NONNULL_END
