#import "MWMRouteStepInfo.h"

#include "routing/route_step.hpp"

@class MWMLaneInfo;

@interface MWMRouteStepInfo (CPP)
- (instancetype)initWithRouteStepInfo:(routing::RouteStepInfo const &)step;
@end

NSArray<MWMLaneInfo *> * MWMBuildLaneInfos(routing::turns::lanes::LanesInfo const & lanes);
