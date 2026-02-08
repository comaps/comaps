#import "PlacePageInfoData+Core.h"

#import "OpeningHours.h"
#import "PlacePagePhone.h"

#import <CoreApi/StringUtils.h>

#include "platform/localization.hpp"

#include "indexer/validate_and_format_contacts.hpp"
#include "indexer/feature_meta.hpp"

#include "map/place_page_info.hpp"

using namespace place_page;
using namespace osm;

/// Get localized metadata value string when string format is "type.feature.value".
NSString * GetLocalizedMetadataValueString(MapObject::MetadataID metaID, std::string const & value) {
  return ToNSString(localisation::TranslatedFeatureType(feature::ToString(metaID) + "." + value));
}

/// Parse date string in YYYY-MM-DD format to NSDate
NSDate * _Nullable ParseDateString(NSString * _Nullable dateString) {
  if (!dateString || dateString.length == 0) {
    return nil;
  }
  
  static NSDateFormatter *dateFormatter = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    dateFormatter = [[NSDateFormatter alloc] init];
    dateFormatter.dateFormat = @"yyyy-MM-dd";
    dateFormatter.locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"];
    dateFormatter.timeZone = [NSTimeZone localTimeZone];
  });
  
  return [dateFormatter dateFromString:dateString];
}

@implementation PlacePageInfoData

- (NSDate * _Nullable)getMostRecentCheckDate {
  // CheckDate can utilize checkDateOpeningHours if that is available
  // As surveying opening hours would confirm presence
  if (_checkDate && _checkDateOpeningHours) {
    // Both available - return the more recent date
    return [_checkDate compare:_checkDateOpeningHours] == NSOrderedDescending ? _checkDate : _checkDateOpeningHours;
  } else if (_checkDate) {
    return _checkDate;
  } else if (_checkDateOpeningHours) {
    return _checkDateOpeningHours;
  } else {
    return nil;
  }
}

@end

@implementation PlacePageInfoData (Core)

- (instancetype)initWithRawData:(Info const &)rawData ohLocalization:(id<IOpeningHoursLocalization>)localization {
  self = [super init];
  if (self)
  {
    auto const cuisines = rawData.FormatCuisines();
    if (!cuisines.empty())
      _cuisine = ToNSString(cuisines);

    /// @todo Refactor PlacePageInfoData to have a map of simple string properties.
    using MetadataID = MapObject::MetadataID;
    rawData.ForEachMetadataReadable([&](MetadataID metaID, std::string const & value)
    {
      switch (metaID)
      {
        case MetadataID::FMD_OPEN_HOURS:
          _openingHoursString = ToNSString(value);
          _openingHours = [[OpeningHours alloc] initWithRawString:_openingHoursString
                                                     localization:localization];
          break;
        case MetadataID::FMD_CHECK_DATE:
          _checkDate = ParseDateString(ToNSString(value));
          break;
        case MetadataID::FMD_CHECK_DATE_OPEN_HOURS:
          _checkDateOpeningHours = ParseDateString(ToNSString(value));
          break;
        case MetadataID::FMD_PHONE_NUMBER:
        {
          NSArray<NSString *> *phones = [ToNSString(value) componentsSeparatedByString:@";"];
          NSMutableArray<PlacePagePhone *> *placePhones = [NSMutableArray new];
          [phones enumerateObjectsUsingBlock:^(NSString * _Nonnull phone, NSUInteger idx, BOOL * _Nonnull stop) {
            NSString *filteredDigits = [[phone componentsSeparatedByCharactersInSet:
                                         [[NSCharacterSet decimalDigitCharacterSet] invertedSet]]
                                        componentsJoinedByString:@""];
            NSString *resultNumber = [phone hasPrefix:@"+"] ? [NSString stringWithFormat:@"+%@", filteredDigits] : filteredDigits;
            NSURL *phoneUrl = [NSURL URLWithString:[NSString stringWithFormat:@"tel://%@", resultNumber]];

            [placePhones addObject:[PlacePagePhone placePagePhoneWithPhone:phone andURL:phoneUrl]];
          }];
          _phones = [placePhones copy];
          break;
        }
        case MetadataID::FMD_WEBSITE: _website = ToNSString(value); break;
        case MetadataID::FMD_WIKIPEDIA: _wikipedia = ToNSString(value); break;
        case MetadataID::FMD_WIKIMEDIA_COMMONS: _wikimediaCommons = ToNSString(value); break;
        case MetadataID::FMD_EMAIL:
          _email = ToNSString(value);
          _emailUrl = [NSURL URLWithString:[NSString stringWithFormat:@"mailto:%@", _email]];
          break;
        case MetadataID::FMD_CONTACT_FEDIVERSE: _fediverse = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_FACEBOOK: _facebook = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_INSTAGRAM: _instagram = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_TWITTER: _twitter = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_VK: _vk = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_LINE: _line = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_BLUESKY: _bluesky = ToNSString(value); break;
        case MetadataID::FMD_PANORAMAX: _panoramax = ToNSString(value); break;
        case MetadataID::FMD_OPERATOR: _ppOperator = [NSString stringWithFormat:NSLocalizedString(@"operator", nil), ToNSString(value)]; break;
        case MetadataID::FMD_BRANCH: _branch = ToNSString(value); break;
        case MetadataID::FMD_INTERNET:
          _wifiAvailable = (rawData.GetInternet() == feature::Internet::No)
              ? NSLocalizedString(@"no_available", nil) : NSLocalizedString(@"yes_available", nil);
          break;
        case MetadataID::FMD_LEVEL: _level = ToNSString(value); break;
        case MetadataID::FMD_CAPACITY: _capacity = [NSString stringWithFormat:NSLocalizedString(@"capacity", nil), ToNSString(value)]; break;
        case MetadataID::FMD_ROOMS: _rooms = [NSString stringWithFormat:NSLocalizedString(@"rooms", nil), ToNSString(value)]; break;
        case MetadataID::FMD_CHARGE: _charge = ToNSString(value); break;
        case MetadataID::FMD_WHEELCHAIR: _wheelchair = ToNSString(localisation::TranslatedFeatureType(value)); break;
        case MetadataID::FMD_DRIVE_THROUGH:
          if (value == "yes")
            _driveThrough = NSLocalizedString(@"drive_through", nil);
          break;
        case MetadataID::FMD_WEBSITE_MENU: _websiteMenu = ToNSString(value); break;
        case MetadataID::FMD_SELF_SERVICE: _selfService = GetLocalizedMetadataValueString(metaID, value); break;
        case MetadataID::FMD_OUTDOOR_SEATING:
          if (value == "yes")
            _outdoorSeating = NSLocalizedString(@"outdoor_seating", nil);
          break;
        case MetadataID::FMD_NETWORK: _network = [NSString stringWithFormat:NSLocalizedString(@"network", nil), ToNSString(value)]; break;
        //TODO: Add space after each 3 digits
        case MetadataID::FMD_POPULATION: _population = [NSString stringWithFormat:NSLocalizedString(@"population", nil), ToNSString(value)]; break;
        default:
          break;
      }
    });

    _atm = rawData.HasAtm() ? NSLocalizedStringFromTable(@"type.amenity.atm", @"LocalizableTypes", nil) : nil;

    _address = rawData.GetSecondarySubtitle().empty() ? nil : @(rawData.GetSecondarySubtitle().c_str());
    _coordFormats = @[@(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDMS).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDecimal).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::OLCFull).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::OSMLink).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::UTM).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::MGRS).c_str())];
  }
  return self;
}

@end
