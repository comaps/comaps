#import "MWMRoutingOptions.h"

#import "MWMStorage.h"

#include "defines.hpp"
#include "routing/routing_options.hpp"

@interface MWMRoutingOptions ()
{
  routing::RoutingOptions _options;
  routing::BorderAvoidanceSettings _borderSettings;
}

@end

@implementation MWMRoutingOptions

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _options = routing::RoutingOptions::LoadCarOptionsFromSettings();
    _borderSettings = routing::BorderAvoidanceSettings::LoadFromSettings();
  }

  return self;
}

- (BOOL)avoidToll
{
  return _options.Has(routing::RoutingOptions::Road::Toll);
}

- (void)setAvoidToll:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Toll) enabled:avoid];
}

- (BOOL)avoidDirty
{
  return _options.Has(routing::RoutingOptions::Road::Dirty);
}

- (void)setAvoidDirty:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Dirty) enabled:avoid];
}

- (BOOL)avoidPaved
{
  return _options.Has(routing::RoutingOptions::Road::Paved);
}

- (void)setAvoidPaved:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Paved) enabled:avoid];
}

- (BOOL)avoidFerry
{
  return _options.Has(routing::RoutingOptions::Road::Ferry);
}

- (void)setAvoidFerry:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Ferry) enabled:avoid];
}

- (BOOL)avoidMotorway
{
  return _options.Has(routing::RoutingOptions::Road::Motorway);
}

- (void)setAvoidMotorway:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Motorway) enabled:avoid];
}

- (BOOL)avoidSteps
{
  return _options.Has(routing::RoutingOptions::Road::Steps);
}

- (void)setAvoidSteps:(BOOL)avoid
{
  [self setOption:(routing::RoutingOptions::Road::Steps) enabled:avoid];
}

- (BOOL)hasOptions
{
  if (self.borderAvoidanceMode != MWMBorderAvoidanceModeNone)
    return YES;
  return self.avoidToll || self.avoidDirty || self.avoidPaved || self.avoidFerry || self.avoidMotorway ||
         self.avoidSteps;
}

- (MWMBorderAvoidanceMode)borderAvoidanceMode
{
  return static_cast<MWMBorderAvoidanceMode>(_borderSettings.GetMode());
}

- (void)setBorderAvoidanceMode:(MWMBorderAvoidanceMode)mode
{
  _borderSettings.SetMode(static_cast<routing::BorderAvoidance>(mode));
}

- (NSSet<NSString *> *)avoidedBorderCountries
{
  auto const & countries = _borderSettings.GetAvoidedCountries();
  NSMutableSet<NSString *> * result = [NSMutableSet setWithCapacity:countries.size()];
  for (auto const & country : countries)
    [result addObject:[NSString stringWithUTF8String:country.c_str()]];
  return [result copy];
}

- (void)setAvoidedBorderCountries:(NSSet<NSString *> *)countries
{
  ankerl::unordered_dense::set<std::string> countrySet;
  for (NSString * country in countries)
    countrySet.insert(std::string([country UTF8String]));
  _borderSettings.SetAvoidedCountries(std::move(countrySet));
}

- (NSArray<NSString *> *)topLevelCountries
{
  NSString * rootId = [[MWMStorage sharedStorage] getRootId];
  NSArray<NSString *> * allCountries = [[MWMStorage sharedStorage] availableCountriesWithParent:rootId];
  NSMutableArray<NSString *> * filtered = [NSMutableArray arrayWithCapacity:allCountries.count];
  for (NSString * country in allCountries)
    if (![country isEqualToString:@WORLD_FILE_NAME] && ![country isEqualToString:@WORLD_COASTS_FILE_NAME])
      [filtered addObject:country];
  return [filtered copy];
}

- (void)save
{
  routing::RoutingOptions::SaveCarOptionsToSettings(_options);
  _borderSettings.SaveToSettings();
}

- (void)setOption:(routing::RoutingOptions::Road)option enabled:(BOOL)enabled
{
  if (enabled)
    _options.Add(option);
  else
    _options.Remove(option);
}

- (BOOL)isEqual:(id)object
{
  if (![object isMemberOfClass:self.class])
    return NO;
  MWMRoutingOptions * another = (MWMRoutingOptions *)object;
  return another.avoidToll == self.avoidToll && another.avoidDirty == self.avoidDirty &&
         another.avoidPaved == self.avoidPaved && another.avoidFerry == self.avoidFerry &&
         another.avoidMotorway == self.avoidMotorway && another.avoidSteps == self.avoidSteps &&
         another.borderAvoidanceMode == self.borderAvoidanceMode &&
         [another.avoidedBorderCountries isEqualToSet:self.avoidedBorderCountries];
}

@end
