@class PlacePageData;
@class PlacePagePhone;
@class ElevationProfileData;

@interface MWMPlacePageManagerHelper : NSObject

+ (void)updateAvailableArea:(CGRect)frame;
+ (void)editPlace;
+ (void)addBusiness;
+ (void)addPlace:(CLLocationCoordinate2D)coordinate;
+ (void)openWebsite:(PlacePageData *)data;
+ (void)openWebsiteMenu:(PlacePageData *)data;
+ (void)openWikipedia:(PlacePageData *)data;
+ (void)openWikimediaCommons:(PlacePageData *)data;
+ (void)openEmail:(PlacePageData *)data;
+ (void)openFediverse:(PlacePageData *)data;
+ (void)openFacebook:(PlacePageData *)data;
+ (void)openInstagram:(PlacePageData *)data;
+ (void)openTwitter:(PlacePageData *)data;
+ (void)openVk:(PlacePageData *)data;
+ (void)openLine:(PlacePageData *)data;
+ (void)openBluesky:(PlacePageData *)data;
+ (void)openPanoramax:(PlacePageData *)data;
+ (void)call:(PlacePagePhone *)phone;
+ (void)showPlaceDescription:(NSString *)htmlString;
+ (void)addBookmark:(PlacePageData *)data;
+ (void)updateBookmark:(PlacePageData *)data color:(MWMBookmarkColor)color category:(MWMMarkGroupID)category;
+ (void)removeBookmark:(PlacePageData *)data;
+ (void)updateTrack:(PlacePageData *)data color:(UIColor *)color category:(MWMMarkGroupID)category;
+ (void)removeTrack:(PlacePageData *)data;
+ (void)editBookmark:(PlacePageData *)data;
+ (void)editTrack:(PlacePageData *)data;
+ (void)searchBookingHotels:(PlacePageData *)data;
+ (void)book:(PlacePageData *)data;
+ (void)routeFrom:(PlacePageData *)data;
+ (void)routeTo:(PlacePageData *)data;
+ (void)routeAddStop:(PlacePageData *)data;
+ (void)routeRemoveStop:(PlacePageData *)data;
+ (void)avoidDirty;
+ (void)avoidFerry;
+ (void)avoidToll;
+ (void)openElevationDifficultPopup:(PlacePageData *)data;

@end
