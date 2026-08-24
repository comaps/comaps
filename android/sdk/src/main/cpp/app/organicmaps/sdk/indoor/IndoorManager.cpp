#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

extern "C"
{
// Levels cross as numbers so the label is formatted once, in the device locale, by the UI.
static void IndoorLevelsChanged(std::vector<double> const & levels, double activeLevel,
                                std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  jni::TScopedLocalRef const jLevels(env, static_cast<jobject>(env->NewDoubleArray(levels.size())));
  env->SetDoubleArrayRegion(static_cast<jdoubleArray>(jLevels.get()), 0, levels.size(), levels.data());
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener, "onLevelsChanged", "([DD)V"), jLevels.get(),
                      activeLevel);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeAddListener(JNIEnv * env, jclass,
                                                                                                jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetIndoorLevelsListener(std::bind(&IndoorLevelsChanged, std::placeholders::_1, std::placeholders::_2,
                                                 jni::make_global_ref(listener)));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeRemoveListener(JNIEnv *, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetIndoorLevelsListener(nullptr);
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeSelectLevel(JNIEnv *, jclass,
                                                                                                   jdouble level)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  return static_cast<jboolean>(g_framework->NativeFramework()->GetIndoorManager().SelectLevel(level));
}

JNIEXPORT jdoubleArray JNICALL
Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeGetViewportLevels(JNIEnv * env, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  auto const levels = g_framework->NativeFramework()->GetIndoorManager().GetViewportLevels();
  jdoubleArray result = env->NewDoubleArray(levels.size());
  env->SetDoubleArrayRegion(result, 0, levels.size(), levels.data());
  return result;
}

JNIEXPORT jdouble JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeGetActiveLevel(JNIEnv *, jclass)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  return g_framework->NativeFramework()->GetIndoorManager().GetActiveLevelValue();
}
}
