#import "DeepLinkRouteStrategyAdapter.h"
#import <CoreApi/Framework.h>
#import "MWMCoreRouterType.h"
#import "MWMRoutePoint+CPP.h"

@implementation DeepLinkRouteStrategyAdapter

- (instancetype)init:(NSURL *)url {
  self = [super init];
  if (self) {
    auto const parsedData = GetFramework().GetParsedRoutingData();
    auto const points = parsedData.m_points;

    for (auto point: points) {
      if (point.m_type == RouteMarkType::Start) {
        _pStart = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:point
                                                            type:MWMRoutePointTypeStart
                                               intermediateIndex:0];
      } else if (point.m_type == RouteMarkType::Finish) {
        _pFinish = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:point
                                                            type:MWMRoutePointTypeFinish
                                               intermediateIndex:0];
      } else if (point.m_type == RouteMarkType::Intermediate) {
        _pIntermediate = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:point
                                                            type:MWMRoutePointTypeIntermediate
                                               intermediateIndex:0];
      }
    }

    if (_pStart && _pFinish) {
      _type = routerType(parsedData.m_type);
    } else if (points.size() == 2) {
      _pStart = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.front()
                                                          type:MWMRoutePointTypeStart
                                             intermediateIndex:0];
      _pFinish = [[MWMRoutePoint alloc] initWithURLSchemeRoutePoint:points.back()
                                                          type:MWMRoutePointTypeFinish
                                             intermediateIndex:0];
      _type = routerType(parsedData.m_type);
    } else {
      return nil;
    }
  }
  return self;
}

@end
