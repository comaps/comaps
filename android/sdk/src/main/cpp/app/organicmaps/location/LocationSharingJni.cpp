#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "location_sharing/location_sharing_types.hpp"
#include "location_sharing/crypto_util.hpp"

#include "base/logging.hpp"

#include <jni.h>

using namespace location_sharing;

extern "C"
{

// Generate session credentials
JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_location_LocationSharingManager_nativeGenerateSessionCredentials(
    JNIEnv * env, jclass)
{
  SessionCredentials creds = SessionCredentials::Generate();

  // Create String array [sessionId, encryptionKey]
  jobjectArray result = env->NewObjectArray(2, env->FindClass("java/lang/String"), nullptr);
  if (!result)
  {
    LOG(LERROR, ("Failed to create result array"));
    return nullptr;
  }

  jstring sessionId = jni::ToJavaString(env, creds.sessionId);
  jstring encryptionKey = jni::ToJavaString(env, creds.encryptionKey);

  env->SetObjectArrayElement(result, 0, sessionId);
  env->SetObjectArrayElement(result, 1, encryptionKey);

  env->DeleteLocalRef(sessionId);
  env->DeleteLocalRef(encryptionKey);

  return result;
}

// Generate share URL
JNIEXPORT jstring JNICALL
Java_app_organicmaps_location_LocationSharingManager_nativeGenerateShareUrl(
    JNIEnv * env, jclass, jstring jSessionId, jstring jEncryptionKey, jstring jServerBaseUrl)
{
  std::string sessionId = jni::ToNativeString(env, jSessionId);
  std::string encryptionKey = jni::ToNativeString(env, jEncryptionKey);
  std::string serverBaseUrl = jni::ToNativeString(env, jServerBaseUrl);

  SessionCredentials creds(sessionId, encryptionKey);
  std::string shareUrl = creds.GenerateShareUrl(serverBaseUrl);

  return jni::ToJavaString(env, shareUrl);
}

// Encrypt payload
JNIEXPORT jstring JNICALL
Java_app_organicmaps_location_LocationSharingManager_nativeEncryptPayload(
    JNIEnv * env, jclass, jstring jEncryptionKey, jstring jPayloadJson)
{
  std::string encryptionKey = jni::ToNativeString(env, jEncryptionKey);
  std::string payloadJson = jni::ToNativeString(env, jPayloadJson);

  auto encryptedOpt = crypto::EncryptAes256Gcm(encryptionKey, payloadJson);
  if (!encryptedOpt.has_value())
  {
    LOG(LERROR, ("Encryption failed"));
    return nullptr;
  }

  std::string resultJson = encryptedOpt->ToJson();
  return jni::ToJavaString(env, resultJson);
}

}  // extern "C"
