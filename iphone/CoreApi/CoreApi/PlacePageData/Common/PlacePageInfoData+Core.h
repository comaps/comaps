#import "PlacePageInfoData.h"

#include <CoreApi/Framework.h>

@protocol IOpeningHoursLocalization;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageInfoData (Core)

+ (NSString *)localizedMetadataCountForKey:(NSString *)localizationKey rawValue:(NSString *)rawValue;

- (instancetype)initWithRawData:(place_page::Info const &)rawData ohLocalization:(id<IOpeningHoursLocalization>)localization;

@end

NS_ASSUME_NONNULL_END
