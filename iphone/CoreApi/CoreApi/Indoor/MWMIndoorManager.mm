#import "MWMIndoorManager.h"

#include "Framework.h"

@interface MWMIndoorManager ()

@property(nonatomic) NSHashTable<id<MWMIndoorObserver>> *observers;
@property(nonatomic) NSArray<NSString *> *currentLevels;
@property(nonatomic) NSString *currentActiveLevel;

@end

@implementation MWMIndoorManager

+ (MWMIndoorManager *)manager {
  static MWMIndoorManager *manager;
  static dispatch_once_t onceToken = 0;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] initManager];
  });
  return manager;
}

- (instancetype)initManager {
  self = [super init];
  if (self) {
    _observers = [NSHashTable weakObjectsHashTable];
    _currentLevels = @[];
    _currentActiveLevel = @"";
    GetFramework().GetIndoorManager().SetLevelsListener(
        [self](std::vector<std::string> const &levels, std::string const &activeLevel) {
          NSMutableArray<NSString *> *result = [NSMutableArray arrayWithCapacity:levels.size()];
          for (auto const &level : levels)
            [result addObject:@(level.c_str())];
          self.currentLevels = result;
          self.currentActiveLevel = @(activeLevel.c_str());
          for (id<MWMIndoorObserver> observer in self.observers)
            [observer onIndoorLevelsUpdated];
        });
  }
  return self;
}

+ (void)addObserver:(id<MWMIndoorObserver>)observer {
  [[MWMIndoorManager manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMIndoorObserver>)observer {
  [[MWMIndoorManager manager].observers removeObject:observer];
}

+ (NSArray<NSString *> *)levels {
  return [MWMIndoorManager manager].currentLevels;
}

+ (NSArray<NSString *> *)viewportLevels {
  auto const levels = GetFramework().GetIndoorManager().GetViewportLevels();
  NSMutableArray<NSString *> *result = [NSMutableArray arrayWithCapacity:levels.size()];
  for (auto const &level : levels)
    [result addObject:@(level.c_str())];
  return result;
}

+ (NSString *)activeLevel {
  // Read straight from the core so the value is correct even before the first observer callback.
  return @(GetFramework().GetIndoorManager().GetActiveLevel().c_str());
}

+ (void)selectLevel:(NSString *)level {
  GetFramework().GetIndoorManager().SelectLevel(level.UTF8String);
}

@end
