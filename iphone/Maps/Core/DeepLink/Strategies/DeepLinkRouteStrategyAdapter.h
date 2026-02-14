#import <Foundation/Foundation.h>
#import "MWMRouterType.h"

NS_ASSUME_NONNULL_BEGIN

@class MWMRoutePoint;
@interface DeepLinkRouteStrategyAdapter : NSObject

@property(nonatomic, readonly) MWMRoutePoint* pStart;
@property(nonatomic, readonly) MWMRoutePoint* pIntermediate;
@property(nonatomic, readonly) MWMRoutePoint* pFinish;
@property(nonatomic, readonly) MWMRouterType type;

- (nullable instancetype)init:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END
