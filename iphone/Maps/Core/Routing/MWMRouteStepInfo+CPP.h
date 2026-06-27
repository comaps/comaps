#import "MWMRouteStepInfo.h"

#include "routing/route_step.hpp"

@interface MWMRouteStepInfo (CPP)
- (instancetype)initWithRouteStepInfo:(routing::RouteStepInfo const &)step;
@end
