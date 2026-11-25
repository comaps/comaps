#import "MWMRoutePlan.h"

#include "routing/route_step.hpp"

@interface MWMRoutePlan (CPP)
- (instancetype)initWithRoutePlan:(routing::RoutePlan const &)routePlan;
@end
