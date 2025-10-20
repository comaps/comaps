#import "LocationSharingBridge.h"

#include "location_sharing/location_sharing_types.hpp"
#include "location_sharing/crypto_util.hpp"

#include "base/logging.hpp"

#import <Foundation/Foundation.h>

using namespace location_sharing;

@implementation LocationSharingBridgeObjC

+ (NSArray<NSString *> *)generateSessionCredentials
{
  SessionCredentials creds = SessionCredentials::Generate();

  NSString * sessionId = [NSString stringWithUTF8String:creds.sessionId.c_str()];
  NSString * encryptionKey = [NSString stringWithUTF8String:creds.encryptionKey.c_str()];

  return @[sessionId, encryptionKey];
}

+ (NSString *)generateShareUrlWithSessionId:(NSString *)sessionId
                              encryptionKey:(NSString *)encryptionKey
                              serverBaseUrl:(NSString *)serverBaseUrl
{
  std::string sessionIdStr = [sessionId UTF8String];
  std::string encryptionKeyStr = [encryptionKey UTF8String];
  std::string serverBaseUrlStr = [serverBaseUrl UTF8String];

  SessionCredentials creds(sessionIdStr, encryptionKeyStr);
  std::string shareUrl = creds.GenerateShareUrl(serverBaseUrlStr);

  return [NSString stringWithUTF8String:shareUrl.c_str()];
}

+ (NSString *)encryptPayloadWithKey:(NSString *)key plaintext:(NSString *)plaintext
{
  std::string keyStr = [key UTF8String];
  std::string plaintextStr = [plaintext UTF8String];

  auto encryptedOpt = crypto::EncryptAes256Gcm(keyStr, plaintextStr);
  if (!encryptedOpt.has_value())
  {
    LOG(LERROR, ("Encryption failed"));
    return nil;
  }

  std::string resultJson = encryptedOpt->ToJson();
  return [NSString stringWithUTF8String:resultJson.c_str()];
}

+ (NSString *)decryptPayloadWithKey:(NSString *)key encryptedJson:(NSString *)encryptedJson
{
  std::string keyStr = [key UTF8String];
  std::string encryptedJsonStr = [encryptedJson UTF8String];

  // Parse encrypted JSON
  auto encryptedPayloadOpt = EncryptedPayload::FromJson(encryptedJsonStr);
  if (!encryptedPayloadOpt.has_value())
  {
    LOG(LERROR, ("Failed to parse encrypted JSON"));
    return nil;
  }

  auto decryptedOpt = crypto::DecryptAes256Gcm(keyStr, *encryptedPayloadOpt);
  if (!decryptedOpt.has_value())
  {
    LOG(LERROR, ("Decryption failed"));
    return nil;
  }

  return [NSString stringWithUTF8String:decryptedOpt->c_str()];
}

@end
