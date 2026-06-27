#import "MWMRouteStepInfo.h"
#import "MWMRouteStepInfo+CPP.h"

@implementation MWMRouteStepInfo

- (instancetype)initWithRouteStepInfo:(routing::RouteStepInfo const &)step
{
  self = [super init];
  if (self)
  {
    _index = step.m_index;
    _carDirection = static_cast<int32_t>(step.m_turn);
    _pedestrianDirection = static_cast<int32_t>(step.m_pedestrianTurn);
    _exitNum = step.m_exitNum;
    _distMeters = step.m_distMeters;
    _formattedDistance = @(step.m_formattedDistance.ToString().c_str());
    _fromStreetName = step.m_fromStreetName.empty() ? nil : @(step.m_fromStreetName.c_str());
    _toStreetName = step.m_toStreetName.empty() ? nil : @(step.m_toStreetName.c_str());
    _textualInstruction = @(step.m_textualInstruction.c_str());
  }
  return self;
}

@end
