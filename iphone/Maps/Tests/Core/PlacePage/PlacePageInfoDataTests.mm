#import <XCTest/XCTest.h>

#import "../../../../CoreApi/CoreApi/PlacePageData/Common/PlacePageInfoData+Core.h"

#include "indexer/feature_meta.hpp"

namespace
{
class TestPlacePageInfo : public place_page::Info
{
public:
  void SetMetadata(feature::Metadata::EType type, std::string value)
  {
    m_metadata.Set(type, std::move(value));
  }
};

PlacePageInfoData * MakePlacePageInfoData(feature::Metadata::EType type, NSString * value)
{
  TestPlacePageInfo rawData;
  rawData.SetMetadata(type, value.UTF8String);
  return [[PlacePageInfoData alloc] initWithRawData:rawData
                                    ohLocalization:(id<IOpeningHoursLocalization>)[NSNull null]];
}

NSString * LocalizedNumber(NSString * value)
{
  NSNumberFormatter * formatter = [[NSNumberFormatter alloc] init];
  formatter.numberStyle = NSNumberFormatterDecimalStyle;
  return [formatter stringFromNumber:@(value.longLongValue)];
}
}  // namespace

@interface PlacePageInfoDataTests : XCTestCase
@end

@implementation PlacePageInfoDataTests

- (void)testLocalizedMetadataCountFormatsAllPluralKeysAndPreserves64BitValues
{
  NSArray<NSString *> * keys = @[@"capacity", @"rooms", @"population", @"capacity_disabled",
                                  @"capacity_charging"];
  NSArray<NSString *> * values = @[@"0", @"1", @"2", @"2147483648"];

  for (NSString * key in keys)
  {
    for (NSString * value in values)
    {
      NSString * result = [PlacePageInfoData localizedMetadataCountForKey:key rawValue:value];
      XCTAssertGreaterThan(result.length, 0U, @"%@=%@", key, value);
      XCTAssertNotEqualObjects(result, key, @"%@=%@", key, value);
      XCTAssertTrue([result containsString:LocalizedNumber(value)], @"%@=%@ produced %@", key, value, result);
    }
  }
}

- (void)testNumericMetadataUsesLocalizedPluralFormatting
{
  struct TestCase
  {
    feature::Metadata::EType m_type;
    __unsafe_unretained NSString * m_property;
  };

  TestCase const testCases[] = {
    {feature::Metadata::FMD_CAPACITY, @"capacity"},
    {feature::Metadata::FMD_ROOMS, @"rooms"},
    {feature::Metadata::FMD_POPULATION, @"population"},
    {feature::Metadata::FMD_CAPACITY_DISABLED, @"capacityDisabled"},
    {feature::Metadata::FMD_CAPACITY_CHARGING, @"capacityCharging"},
  };

  for (auto const & testCase : testCases)
  {
    for (NSString * value in @[@"1", @"2", @"2147483648"])
    {
      PlacePageInfoData * data = MakePlacePageInfoData(testCase.m_type, value);
      NSString * result = [data valueForKey:testCase.m_property];
      XCTAssertGreaterThan(result.length, 0U, @"%@=%@", testCase.m_property, value);
      XCTAssertTrue([result containsString:LocalizedNumber(value)], @"%@=%@ produced %@", testCase.m_property,
                    value, result);
    }
  }
}

- (void)testCapacityDisabledAndChargingPreserveSpecialValues
{
  struct TestCase
  {
    feature::Metadata::EType m_type;
    __unsafe_unretained NSString * m_property;
    __unsafe_unretained NSString * m_yesKey;
    __unsafe_unretained NSString * m_noKey;
  };

  TestCase const testCases[] = {
    {feature::Metadata::FMD_CAPACITY_DISABLED, @"capacityDisabled", @"capacity_disabled_yes",
     @"capacity_disabled_no"},
    {feature::Metadata::FMD_CAPACITY_CHARGING, @"capacityCharging", @"capacity_charging_yes",
     @"capacity_charging_no"},
  };

  for (auto const & testCase : testCases)
  {
    PlacePageInfoData * yesData = MakePlacePageInfoData(testCase.m_type, @"yes");
    XCTAssertEqualObjects([yesData valueForKey:testCase.m_property], NSLocalizedString(testCase.m_yesKey, nil));

    for (NSString * value in @[@"no", @"0"])
    {
      PlacePageInfoData * noData = MakePlacePageInfoData(testCase.m_type, value);
      XCTAssertEqualObjects([noData valueForKey:testCase.m_property], NSLocalizedString(testCase.m_noKey, nil));
    }
  }
}

@end
