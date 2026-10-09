#import "MWMRouteStepInfo.h"
#import "MWMRouteStepInfo+CPP.h"
#import "SwiftBridge.h"

NSArray<MWMLaneInfo *> * MWMBuildLaneInfos(routing::turns::lanes::LanesInfo const & lanes)
{
  NSMutableArray<MWMLaneInfo *> * result = [NSMutableArray arrayWithCapacity:lanes.size()];
  for (auto const & lane : lanes)
  {
    auto const activeWays = lane.laneWays.GetActiveLaneWays();
    NSMutableArray<NSNumber *> * laneWays = [NSMutableArray arrayWithCapacity:activeWays.size()];
    for (auto const way : activeWays)
      [laneWays addObject:@(static_cast<uint8_t>(way))];
    [result addObject:[[MWMLaneInfo alloc] initWithLaneWays:laneWays
                                             recommendedWay:static_cast<uint8_t>(lane.recommendedWay)]];
  }
  return result;
}

@implementation MWMRouteStepInfo

- (instancetype)initWithRouteStepInfo:(routing::RouteStepInfo const &)step
{
  self = [super init];
  if (self)
  {
    _index = step.m_index;
    _turnIndex = step.m_turnIndex;
    _carDirection = static_cast<int32_t>(step.m_turn);
    _pedestrianDirection = static_cast<int32_t>(step.m_pedestrianTurn);
    _exitNum = step.m_exitNum;
    _distMeters = step.m_distMeters;
    _formattedDistance = @(step.m_formattedDistance.ToString().c_str());
    _fromStreetName = step.m_fromStreetName.empty() ? nil : @(step.m_fromStreetName.c_str());
    _toStreetName = step.m_toStreetName.empty() ? nil : @(step.m_toStreetName.c_str());
    _toRef = @(step.m_toRef.c_str());
    _toJunctionRef = @(step.m_toJunctionRef.c_str());
    _toDestinationRef = @(step.m_toDestinationRef.c_str());
    _toDestination = @(step.m_toDestination.c_str());
    _toIsLink = step.m_toIsLink;
    _lanes = MWMBuildLaneInfos(step.m_lanes);
    _textualInstruction = @(step.m_textualInstruction.c_str());
  }
  return self;
}

@end
