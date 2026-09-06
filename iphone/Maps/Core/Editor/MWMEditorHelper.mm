#import "MWMEditorHelper.h"
#import <CoreApi/AppInfo.h>
#import "SwiftBridge.h"

#include <string>
#include <map>
#include <functional>
#include "editor/osm_editor.hpp"

@implementation MWMEditorHelper

+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler
{
  if (!Profile.isExisting ||
      Platform::EConnectionType::CONNECTION_NONE == Platform::ConnectionStatus())
  {
    completionHandler(UIBackgroundFetchResultFailed);
  }
  else
  {
    auto const lambda = [completionHandler](osm::Editor::UploadResult result) {
      switch (result)
      {
      case osm::Editor::UploadResult::Success:
        completionHandler(UIBackgroundFetchResultNewData);
        break;
      case osm::Editor::UploadResult::Error:
        completionHandler(UIBackgroundFetchResultFailed);
        break;
      case osm::Editor::UploadResult::NothingToUpload:
        completionHandler(UIBackgroundFetchResultNoData);
        break;
      }
    };
    
    NSString *authorizationToken = Profile.authorizationToken;
    if (authorizationToken == nil) {
      authorizationToken = @"";
    }
    std::string const oauthToken = std::string([authorizationToken UTF8String]);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
      osm::Editor::Instance().UploadChanges(
          oauthToken,
          {{"created_by", std::string("CoMaps " OMIM_OS_NAME " ") + AppInfo.sharedInfo.bundleVersion.UTF8String},
           {"bundle_id", NSBundle.mainBundle.bundleIdentifier.UTF8String}},
          lambda);
    });
  }
}

@end
