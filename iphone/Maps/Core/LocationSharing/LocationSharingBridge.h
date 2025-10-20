#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Objective-C++ bridge to C++ location sharing functionality
@interface LocationSharingBridgeObjC : NSObject

/// Generate new session credentials
/// @return Array of [sessionId, encryptionKey]
+ (NSArray<NSString *> *)generateSessionCredentials;

/// Generate share URL from credentials
+ (nullable NSString *)generateShareUrlWithSessionId:(NSString *)sessionId
                                       encryptionKey:(NSString *)encryptionKey
                                       serverBaseUrl:(NSString *)serverBaseUrl;

/// Encrypt payload using AES-256-GCM
/// @param key Base64-encoded encryption key
/// @param plaintext JSON payload to encrypt
/// @return Encrypted JSON (with iv, ciphertext, authTag) or nil on failure
+ (nullable NSString *)encryptPayloadWithKey:(NSString *)key plaintext:(NSString *)plaintext;

/// Decrypt payload using AES-256-GCM
/// @param key Base64-encoded encryption key
/// @param encryptedJson Encrypted JSON (with iv, ciphertext, authTag)
/// @return Decrypted plaintext or nil on failure
+ (nullable NSString *)decryptPayloadWithKey:(NSString *)key encryptedJson:(NSString *)encryptedJson;

@end

NS_ASSUME_NONNULL_END
