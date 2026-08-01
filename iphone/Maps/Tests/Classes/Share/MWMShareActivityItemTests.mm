#import <XCTest/XCTest.h>
#import "MWMShareActivityItem.h"

#include "ge0/url_generator.hpp"

extern NSString * httpGe0Url(NSString * shortUrl);

#pragma mark - Unit

@interface MWMShareActivityItemTests : XCTestCase
@end

@implementation MWMShareActivityItemTests

- (void)testHttpGe0UrlReplacesPrefix {
    XCTAssertEqualObjects(httpGe0Url(@"comaps://o4CoCoCoCo/Test_Location"), @"https://comaps.at/o4CoCoCoCo/Test_Location");
}

@end

#pragma mark - Integration

@interface ShareUrlIntegrationTests : XCTestCase
@end

@implementation ShareUrlIntegrationTests

- (void)testGenerateShortShowMapUrlUsesCoMapsPrefix {
    std::string const url = ge0::GenerateShortShowMapUrl(50.0, 0.0, 14.0, "Test Location");
    
    NSString * const shortUrl = @(url.c_str());
    
    XCTAssertTrue([shortUrl hasPrefix:@"comaps://"], @"Expected 'comaps://' prefix, got: %@", shortUrl);
}

- (void)testStringIsFormattedCorrectly {
    std::string const url = ge0::GenerateShortShowMapUrl(50.0, 0.0, 14.0, "Test Location");
    
    NSString * const httpUrl = httpGe0Url(@(url.c_str()));
    
    XCTAssertEqualObjects(httpUrl, @"https://comaps.at/o4CoCoCoCo/Test_Location");
}

@end
