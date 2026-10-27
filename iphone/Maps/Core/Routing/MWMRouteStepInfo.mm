#import "MWMRouteStepInfo.h"
#import "MWMRouteStepInfo+CPP.h"
#import "MWMLaneInfo+CPP.h"

@implementation MWMRouteStepInfo

- (instancetype)initWithRouteStepInfo:(routing::RouteStepInfo const &)step
{
  self = [super init];
  if (self)
  {
    _index = step.m_index;
    _sequence = step.m_sequence;
    _carDirection = static_cast<int32_t>(step.m_turn);
    _pedestrianDirection = static_cast<int32_t>(step.m_pedestrianTurn);
    _exitNum = step.m_exitNum;
    _distMeters = step.m_distMeters;
    _formattedDistance = @(step.m_formattedDistance.ToString().c_str());
    _fromStreetName = step.m_fromStreetName.empty() ? nil : @(step.m_fromStreetName.c_str());
    _toStreetName = step.m_toRoad.m_name.empty() ? nil : @(step.m_toRoad.m_name.c_str());
    _toRef = @(step.m_toRoad.m_ref.c_str());
    _toJunctionRef = @(step.m_toRoad.m_junctionRef.c_str());
    _toDestinationRef = @(step.m_toRoad.m_destinationRef.c_str());
    _toDestination = @(step.m_toRoad.m_destination.c_str());
    _toIsLink = step.m_toRoad.m_isLink;
    _lanes = MWMBuildLaneInfos(step.m_lanes);
    _textualInstruction = @(step.m_textualInstruction.c_str());
  }
  return self;
}

@end
