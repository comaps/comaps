#import "MWMRoutePlan+CPP.h"

#import "MWMRouteStepInfo+CPP.h"

@implementation MWMRoutePlan

- (instancetype)initWithRoutePlan:(routing::RoutePlan const &)routePlan
{
    self = [super init];
    if (self)
    {
        _routeID = routePlan.m_routeId;
        NSMutableArray<MWMRouteStepInfo *> *steps = [NSMutableArray arrayWithCapacity:routePlan.m_steps.size()];
        for (auto const & step : routePlan.m_steps)
        {
            [steps addObject:[[MWMRouteStepInfo alloc] initWithRouteStepInfo:step]];
        }
        _steps = [steps copy];
    }
    return self;
}

@end
