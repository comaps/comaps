#import "DurationFormatter.h"

#include "platform/duration.hpp"

#include "i18n/duration.hpp"

@implementation DurationFormatter

+ (NSString *)durationStringFromTimeInterval:(NSTimeInterval)timeInterval {
  auto const duration = platform::Duration(static_cast<int>(timeInterval));
  return [NSString stringWithCString:platform::GetPlatformLocalizedString(duration).c_str() encoding:NSUTF8StringEncoding];
}

@end
