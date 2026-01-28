#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "platform/settings.hpp"

extern "C"
{
JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_Config_nativeHasConfigValue(JNIEnv * env, jclass thiz, jstring name)
{
  std::string value;
  return settings::Get(jni::ToNativeString(env, name), value);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeDeleteConfigValue(JNIEnv * env, jclass thiz, jstring name)
{
  settings::Delete(jni::ToNativeString(env, name));
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetBoolean(JNIEnv * env, jclass thiz, jstring name, jboolean defaultVal)
{
  bool val;
  if (settings::Get(jni::ToNativeString(env, name), val))
    return static_cast<jboolean>(val);

  return defaultVal;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetBoolean(JNIEnv * env, jclass thiz, jstring name, jboolean val)
{
  (void)settings::Set(jni::ToNativeString(env, name), static_cast<bool>(val));
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetInt(JNIEnv * env, jclass thiz, jstring name, jint defaultValue)
{
  int32_t value;
  if (settings::Get(jni::ToNativeString(env, name), value))
    return static_cast<jint>(value);

  return defaultValue;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetInt(JNIEnv * env, jclass thiz, jstring name, jint value)
{
  (void)settings::Set(jni::ToNativeString(env, name), static_cast<int32_t>(value));
}

JNIEXPORT jlong JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetLong(JNIEnv * env, jclass thiz, jstring name, jlong defaultValue)
{
  int64_t value;
  if (settings::Get(jni::ToNativeString(env, name), value))
    return static_cast<jlong>(value);

  return defaultValue;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetLong(JNIEnv * env, jclass thiz, jstring name, jlong value)
{
  (void)settings::Set(jni::ToNativeString(env, name), static_cast<int64_t>(value));
}

JNIEXPORT jdouble JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetDouble(JNIEnv * env, jclass thiz, jstring name, jdouble defaultValue)
{
  double value;
  if (settings::Get(jni::ToNativeString(env, name), value))
    return static_cast<jdouble>(value);

  return defaultValue;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetDouble(JNIEnv * env, jclass thiz, jstring name, jdouble value)
{
  (void)settings::Set(jni::ToNativeString(env, name), static_cast<double>(value));
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetString(JNIEnv * env, jclass thiz, jstring name, jstring defaultValue)
{
  std::string value;
  if (settings::Get(jni::ToNativeString(env, name), value))
    return jni::ToJavaString(env, value);

  return defaultValue;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetString(JNIEnv * env, jclass thiz, jstring name, jstring value)
{
  (void)settings::Set(jni::ToNativeString(env, name), jni::ToNativeString(env, value));
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetLargeFontsSize(JNIEnv * env, jclass thiz)
{
  return frm()->LoadLargeFontsSize();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetLargeFontsSize(JNIEnv * env, jclass thiz, jboolean value)
{
  frm()->SetLargeFontsSize(value);
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetOnlyUseSysLangsInTheirRegion(JNIEnv * env, jclass thiz)
{
  bool enabled = true;
  UNUSED_VALUE(settings::Get(settings::kMapLanguageLimitAlternativesToLocal, enabled));
  return enabled;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetOnlyUseSysLangsInTheirRegion(JNIEnv * env, jclass thiz, jboolean value)
{
  settings::Set(settings::kMapLanguageLimitAlternativesToLocal, static_cast<bool>(value));
  //reload render
  frm()->InvalidateRect(frm()->GetCurrentViewport());
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_Config_nativeGetTransliteration(JNIEnv * env, jclass thiz)
{
  return frm()->LoadTransliteration();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_Config_nativeSetTransliteration(JNIEnv * env, jclass thiz, jboolean value)
{
  frm()->SaveTransliteration(value);
  frm()->AllowTransliteration(value);
}

JNIEXPORT jboolean JNICALL
Java_app_organicmaps_sdk_util_Config_nativeGetTrafficHttpEnabled(JNIEnv * env, jclass thiz)
{
  return frm()->LoadTrafficHttpEnabled();
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_util_Config_nativeSetTrafficHttpEnabled(JNIEnv * env, jclass thiz,
                                                                 jboolean value)
{
  frm()->SaveTrafficHttpEnabled(value);
  frm()->SetTrafficHttpEnabled(value);
}

JNIEXPORT jstring JNICALL
Java_app_organicmaps_sdk_util_Config_nativeGetTrafficHttpUrl(JNIEnv * env, jclass thiz)
{
  std::string value = frm()->LoadTrafficHttpUrl();
  return jni::ToJavaString(env, value);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_util_Config_nativeSetTrafficHttpUrl(JNIEnv * env, jclass thiz,
                                                             jstring value)
{
  frm()->SaveTrafficHttpUrl(jni::ToNativeString(env, value));
  frm()->SetTrafficHttpUrl(jni::ToNativeString(env, value));
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_util_Config_applyTrafficLegacyEnabled(JNIEnv * env, jclass thiz,
                                                               jboolean value)
{
  TrafficManager & tm = g_framework->GetTrafficManager();
  tm.RemoveTraffSourceIf([](traffxml::TraffSource* source) {
    if (traffxml::AndroidTraffSourceV0_7* traffSource = dynamic_cast<traffxml::AndroidTraffSourceV0_7*>(source))
    {
      traffSource->Close();
      return true;
    }
    else
      return false;
  });
  if (value)
    traffxml::AndroidTraffSourceV0_7::Create(tm);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_util_Config_applyTrafficApps(JNIEnv * env, jclass thiz, jobjectArray value)
{
  jsize valueLen = env->GetArrayLength(value);
  TrafficManager & tm = g_framework->GetTrafficManager();
  tm.RemoveTraffSourceIf([](traffxml::TraffSource* source) {
    if (traffxml::AndroidTraffSourceV0_8* traffSource = dynamic_cast<traffxml::AndroidTraffSourceV0_8*>(source))
    {
      traffSource->Close();
      return true;
    }
    else
      return false;
  });
  for (jsize i = 0; i < valueLen; i++)
  {
    jstring jAppId = (jstring)env->GetObjectArrayElement(value, i);
    std::string appId = jni::ToNativeString(env, jAppId);
    traffxml::AndroidTraffSourceV0_8::Create(tm, appId);
    env->DeleteLocalRef(jAppId);
  }
}
}  // extern "C"
