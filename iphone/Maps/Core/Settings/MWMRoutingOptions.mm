#import "MWMRoutingOptions.h"

#include "routing/routing_options.hpp"

@interface MWMRoutingOptions() {
  routing::RoutingOptions _options;
}

@end

@implementation MWMRoutingOptions

- (instancetype)init {
  self = [super init];
  if (self) {
    _options = routing::RoutingOptions::LoadOptionsFromSettings(routing::VehicleType::Car);
  }

  return self;
}

- (BOOL)avoidToll {
  return _options.Has(routing::RoutingOptions::Option::Toll);
}

- (void)setAvoidToll:(BOOL)avoid {
  [self setOption:(routing::RoutingOptions::Option::Toll) enabled:avoid];
}

- (BOOL)avoidDirty {
  return _options.Has(routing::RoutingOptions::Option::Dirty);
}

- (void)setAvoidDirty:(BOOL)avoid {
  [self setOption:(routing::RoutingOptions::Option::Dirty) enabled:avoid];
}

- (BOOL)avoidPaved {
  return _options.Has(routing::RoutingOptions::Option::Paved);
}

- (void)setAvoidPaved:(BOOL)avoid {
  [self setOption:(routing::RoutingOptions::Option::Paved) enabled:avoid];
}

- (BOOL)avoidFerry {
  return _options.Has(routing::RoutingOptions::Option::Ferry);
}

- (void)setAvoidFerry:(BOOL)avoid {
  [self setOption:(routing::RoutingOptions::Option::Ferry) enabled:avoid];
}

- (BOOL)avoidMotorway {
  return _options.Has(routing::RoutingOptions::Option::Motorway);
}

- (void)setAvoidMotorway:(BOOL)avoid {
  [self setOption:(routing::RoutingOptions::Option::Motorway) enabled:avoid];
}

- (BOOL)avoidSteps {
  return _options.Has(routing::RoutingOptions::Option::Steps);
}

- (void)setAvoidSteps:(BOOL)avoid {
  [self setOption:(routing::RoutingOptions::Option::Steps) enabled:avoid];
}

- (BOOL)hasOptions {
  return self.avoidToll || self.avoidDirty || self.avoidPaved|| self.avoidFerry || self.avoidMotorway || self.avoidSteps;
}

- (void)save {
  _options.setVehicleType(routing::VehicleType::Pedestrian);
  routing::RoutingOptions::SaveOptionsToSettings(_options);
  _options.setVehicleType(routing::VehicleType::Bicycle);
  routing::RoutingOptions::SaveOptionsToSettings(_options);
  _options.setVehicleType(routing::VehicleType::Car);
  routing::RoutingOptions::SaveOptionsToSettings(_options);
  _options.setVehicleType(routing::VehicleType::Transit);
  routing::RoutingOptions::SaveOptionsToSettings(_options);
}

- (void)setOption:(routing::RoutingOptions::Option)option enabled:(BOOL)enabled
{
  if (enabled) {
    _options.Add(option);
  } else {
    _options.Remove(option);
  }
}

- (BOOL)isEqual:(id)object {
  if (![object isMemberOfClass:self.class]) {
    return NO;
  }
  MWMRoutingOptions *another = (MWMRoutingOptions *)object;
  return another.avoidToll == self.avoidToll && another.avoidDirty == self.avoidDirty && another.avoidPaved == self.avoidPaved && another.avoidFerry == self.avoidFerry && another.avoidMotorway == self.avoidMotorway && another.avoidSteps == self.avoidSteps;
}

@end
