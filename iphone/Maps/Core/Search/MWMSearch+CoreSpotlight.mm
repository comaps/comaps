#import <CoreSpotlight/CoreSpotlight.h>
#import <MobileCoreServices/MobileCoreServices.h>
#import <CoreApi/Framework.h>
#import <CoreApi/AppInfo.h>
#import <CoreApi/MWMCommon.h>
#import "MWMSearch+CoreSpotlight.h"
#import "MWMSettings.h"

#include "search/displayed_categories.hpp"

@implementation MWMSearch (CoreSpotlight)

+ (void)addCategoriesToSpotlight
{
  if (isIOSVersionLessThan(9) || ![CSSearchableIndex isIndexingAvailable])
    return;

  NSString * localeLanguageId = [[AppInfo sharedInfo] languageId];
  if ([localeLanguageId isEqualToString:[MWMSettings spotlightLocaleLanguageId]])
    return;

  auto const & categories = GetFramework().GetDisplayedCategories();
  auto const & categoriesKeys = categories.GetKeys();
  NSMutableArray<CSSearchableItem *> * items = [@[] mutableCopy];

  for (auto const & categoryKey : categoriesKeys)
  {
    CSSearchableItemAttributeSet * attrSet = [[CSSearchableItemAttributeSet alloc]
        initWithItemContentType: UTTypeItem.identifier];

    NSString * categoryName = nil;
    NSMutableDictionary<NSString *, NSString *> * localizedStrings = [@{} mutableCopy];

    categories.ForEachSynonym(categoryKey, [&localizedStrings, &localeLanguageId, &categoryName](
                                               std::string const & name, std::string const & locale) {
      NSString * nsName = @(name.c_str());
      NSString * nsLocale = @(locale.c_str());
      if ([localeLanguageId isEqualToString:nsLocale])
        categoryName = nsName;
      localizedStrings[nsLocale] = nsName;
    });
    attrSet.alternateNames = localizedStrings.allValues;
    attrSet.keywords = localizedStrings.allValues;
    attrSet.title = categoryName;
    attrSet.displayName = [[CSLocalizedString alloc] initWithLocalizedStrings:localizedStrings];

    NSString * categoryKeyString = @(categoryKey.c_str());
    NSString * imageName = [NSString stringWithFormat:@"Search/Categories/%@", [categoryKeyString stringByReplacingOccurrencesOfString: @"category_" withString:@""]];
    UIImage * image = [UIImage imageNamed:imageName inBundle:nil compatibleWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle: UIUserInterfaceStyleLight]];
    UIGraphicsBeginImageContext(CGSizeMake(360, 360));
    [image drawInRect:CGRectMake(0, 0, 360, 360)];
    UIImage * resizedImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext() ;
    attrSet.thumbnailData = UIImagePNGRepresentation(resizedImage);

    CSSearchableItem * item =
        [[CSSearchableItem alloc] initWithUniqueIdentifier:categoryKeyString
                                          domainIdentifier:@"comaps.app.categories"
                                              attributeSet:attrSet];
    [items addObject:item];
  }

  [[CSSearchableIndex defaultSearchableIndex]
      indexSearchableItems:items
         completionHandler:^(NSError * _Nullable error) {
           if (error)
           {
             NSError * err = error;
             LOG(LERROR,
                 ("addCategoriesToSpotlight failed: ", err.localizedDescription.UTF8String));
           }
           else
           {
             LOG(LINFO, ("addCategoriesToSpotlight succeded"));
             [MWMSettings setSpotlightLocaleLanguageId:localeLanguageId];
           }
         }];
}

@end
