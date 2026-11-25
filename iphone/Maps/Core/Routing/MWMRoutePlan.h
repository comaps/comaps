#import <Foundation/Foundation.h>

@class MWMRouteStepInfo;

@interface MWMRoutePlan : NSObject

@property(nonatomic, readonly) uint64_t routeID;
@property(nonatomic, readonly) NSArray<MWMRouteStepInfo *> *steps;

@end
