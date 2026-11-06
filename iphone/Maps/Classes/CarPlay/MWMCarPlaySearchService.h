NS_ASSUME_NONNULL_BEGIN
@class MWMCarPlaySearchResultObject;

NS_SWIFT_NAME(CarPlaySearchService)
@interface MWMCarPlaySearchService : NSObject

- (instancetype)init;

- (void)searchText:(NSString *)text
    forInputLocale:(NSString *)inputLocale
 completionHandler:(void (^)(NSArray<MWMCarPlaySearchResultObject *> *searchResults))completionHandler;
- (void)saveLastQuery;
@end

NS_ASSUME_NONNULL_END
