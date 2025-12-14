#include "android/sdk/src/main/cpp/app/organicmaps/sdk/core/ScopedLocalRef.hpp"
#include "android/sdk/src/main/cpp/app/organicmaps/sdk/core/jni_helper.hpp"

#include "platform/locale.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <string>

std::vector<std::string> GetAndroidSystemLanguages()
{
  static char const * DEFAULT_LANG = "en";

  JNIEnv * env = jni::GetEnv();
  if (!env)
  {
    LOG(LWARNING, ("Can't get JNIEnv"));
    return {DEFAULT_LANG};
  }

  static jclass const languageClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/util/Language");

  static jmethodID const getSystemLocalesId =
      jni::GetStaticMethodID(env, languageClass, "getSystemLocales", "()[Ljava/lang/String;");

  jni::TScopedLocalRef resultArray(env, env->CallStaticObjectMethod(languageClass, getSystemLocalesId));

  jobjectArray array = static_cast<jobjectArray>(resultArray.get());
  size_t len = env->GetArrayLength(array);

  std::vector<std::string> languages;
  languages.reserve(len);

  for (size_t i = 0; i < len; ++i)
  {
    jni::TScopedLocalRef elem(env, env->GetObjectArrayElement(array, i));
    std::string lang = jni::ToNativeString(env, static_cast<jstring>(elem.get()));

    languages.push_back(lang);
  }

  return languages;
}

namespace platform
{
Locale GetCurrentLocale()
{
  JNIEnv * env = jni::GetEnv();
  static jmethodID const getLanguageCodeId =
      jni::GetStaticMethodID(env, g_utilsClazz, "getLanguageCode", "()Ljava/lang/String;");
  jni::ScopedLocalRef languageCode(env, env->CallStaticObjectMethod(g_utilsClazz, getLanguageCodeId));

  static jmethodID const getCountryCodeId =
      jni::GetStaticMethodID(env, g_utilsClazz, "getCountryCode", "()Ljava/lang/String;");
  jni::ScopedLocalRef countryCode(env, env->CallStaticObjectMethod(g_utilsClazz, getCountryCodeId));

  static jmethodID const getCurrencyCodeId =
      jni::GetStaticMethodID(env, g_utilsClazz, "getCurrencyCode", "()Ljava/lang/String;");
  jni::ScopedLocalRef currencyCode(env, env->CallStaticObjectMethod(g_utilsClazz, getCurrencyCodeId));

  static jmethodID const getDecimalSeparatorId =
      jni::GetStaticMethodID(env, g_utilsClazz, "getDecimalSeparator", "()Ljava/lang/String;");
  jni::ScopedLocalRef decimalSeparatorChar(env, env->CallStaticObjectMethod(g_utilsClazz, getDecimalSeparatorId));

  static jmethodID const getGroupingSeparatorId =
      jni::GetStaticMethodID(env, g_utilsClazz, "getGroupingSeparator", "()Ljava/lang/String;");
  jni::ScopedLocalRef groupingSeparatorChar(env, env->CallStaticObjectMethod(g_utilsClazz, getGroupingSeparatorId));

  return {jni::ToNativeString(env, static_cast<jstring>(languageCode.get())),
          jni::ToNativeString(env, static_cast<jstring>(countryCode.get())),
          currencyCode.get() ? jni::ToNativeString(env, static_cast<jstring>(currencyCode.get())) : "",
          jni::ToNativeString(env, static_cast<jstring>(decimalSeparatorChar.get())),
          jni::ToNativeString(env, static_cast<jstring>(groupingSeparatorChar.get()))};
}
}  // namespace platform
