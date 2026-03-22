#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/routing_options.hpp"

#include "defines.hpp"

routing::RoutingOptions::Road makeValue(jint option)
{
  auto const road = static_cast<uint8_t>(1u << static_cast<int>(option));
  CHECK_LESS(road, static_cast<uint8_t>(routing::RoutingOptions::Road::Max), ());
  return static_cast<routing::RoutingOptions::Road>(road);
}

extern "C"
{
JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeHasOption(JNIEnv *, jclass,
                                                                                           jint option)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
  routing::RoutingOptions::Road road = makeValue(option);
  return static_cast<jboolean>(routingOptions.Has(road));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeAddOption(JNIEnv *, jclass, jint option)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
  routing::RoutingOptions::Road road = makeValue(option);
  routingOptions.Add(road);
  routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeRemoveOption(JNIEnv *, jclass, jint option)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadCarOptionsFromSettings();
  routing::RoutingOptions::Road road = makeValue(option);
  routingOptions.Remove(road);
  routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeGetBorderAvoidanceMode(JNIEnv *, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const settings = routing::BorderAvoidanceSettings::LoadFromSettings();
  auto const mode = static_cast<jint>(settings.GetMode());
  LOG(LINFO, ("nativeGetBorderAvoidanceMode:", mode));
  return mode;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeSetBorderAvoidanceMode(JNIEnv *, jclass,
                                                                                                    jint mode)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  LOG(LINFO, ("nativeSetBorderAvoidanceMode:", mode));
  auto settings = routing::BorderAvoidanceSettings::LoadFromSettings();
  settings.SetMode(static_cast<routing::BorderAvoidance>(mode));
  settings.SaveToSettings();
}

JNIEXPORT jobjectArray JNICALL
Java_app_organicmaps_sdk_routing_RoutingOptions_nativeGetAvoidedBorderCountries(JNIEnv * env, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const settings = routing::BorderAvoidanceSettings::LoadFromSettings();
  auto const & countries = settings.GetAvoidedCountries();

  jclass stringClass = env->FindClass("java/lang/String");
  jobjectArray result = env->NewObjectArray(static_cast<jsize>(countries.size()), stringClass, nullptr);

  jsize idx = 0;
  for (auto const & country : countries)
  {
    jstring jStr = env->NewStringUTF(country.c_str());
    env->SetObjectArrayElement(result, idx, jStr);
    env->DeleteLocalRef(jStr);
    ++idx;
  }

  return result;
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeSetAvoidedBorderCountries(
    JNIEnv * env, jclass, jobjectArray countries)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto settings = routing::BorderAvoidanceSettings::LoadFromSettings();

  ankerl::unordered_dense::set<std::string> countrySet;
  if (countries != nullptr)
  {
    jsize const len = env->GetArrayLength(countries);
    for (jsize i = 0; i < len; ++i)
    {
      auto jStr = static_cast<jstring>(env->GetObjectArrayElement(countries, i));
      if (jStr != nullptr)
      {
        char const * str = env->GetStringUTFChars(jStr, nullptr);
        if (str != nullptr)
        {
          countrySet.insert(str);
          env->ReleaseStringUTFChars(jStr, str);
        }
        env->DeleteLocalRef(jStr);
      }
    }
  }

  settings.SetAvoidedCountries(std::move(countrySet));
  settings.SaveToSettings();
}

JNIEXPORT jobjectArray JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeGetTopLevelCountries(JNIEnv * env,
                                                                                                          jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));

  try
  {
    auto const & storage = g_framework->GetStorage();
    storage::CountriesVec children;
    storage.GetChildren(storage.GetRootId(), children);

    storage::CountriesVec filtered;
    for (auto const & child : children)
      if (child != WORLD_FILE_NAME && child != WORLD_COASTS_FILE_NAME)
        filtered.push_back(child);

    LOG(LINFO, ("nativeGetTopLevelCountries:", filtered.size(), "countries"));
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(static_cast<jsize>(filtered.size()), stringClass, nullptr);

    jsize idx = 0;
    for (auto const & child : filtered)
    {
      jstring jStr = env->NewStringUTF(child.c_str());
      env->SetObjectArrayElement(result, idx, jStr);
      env->DeleteLocalRef(jStr);
      ++idx;
    }

    return result;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Failed to load countries list:", e.what()));
    return nullptr;
  }
}
}
