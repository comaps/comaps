#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

extern "C"
{
static void IndoorLevelsChanged(std::vector<std::string> const & levels, std::string const & activeLevel,
                                std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  jobjectArray jLevels = jni::ToJavaStringArray(env, levels);
  jni::TScopedLocalRef const jActiveLevel(env, jni::ToJavaString(env, activeLevel));
  env->CallVoidMethod(*listener,
                      jni::GetMethodID(env, *listener, "onLevelsChanged", "([Ljava/lang/String;Ljava/lang/String;)V"),
                      jLevels, jActiveLevel.get());
  env->DeleteLocalRef(jLevels);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeAddListener(JNIEnv * env,
                                                                                                jclass clazz,
                                                                                                jobject listener)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetIndoorLevelsListener(std::bind(&IndoorLevelsChanged, std::placeholders::_1, std::placeholders::_2,
                                                 jni::make_global_ref(listener)));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeRemoveListener(JNIEnv * env,
                                                                                                   jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->SetIndoorLevelsListener(nullptr);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeSelectLevel(JNIEnv * env,
                                                                                                jclass clazz,
                                                                                                jstring level)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  g_framework->NativeFramework()->GetIndoorManager().SelectLevel(jni::ToNativeString(env, level));
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeGetActiveLevel(JNIEnv * env,
                                                                                                      jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  return jni::ToJavaString(env, g_framework->NativeFramework()->GetIndoorManager().GetActiveLevel());
}

JNIEXPORT jobjectArray JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeGetViewportLevels(
    JNIEnv * env, jclass clazz)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  return jni::ToJavaStringArray(env, g_framework->NativeFramework()->GetIndoorManager().GetViewportLevels());
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeIsDebugEnabled(
    JNIEnv * env, jclass clazz)
{
  if (!g_framework) return JNI_FALSE;
  return g_framework->NativeFramework()->GetIndoorManager().IsDebugEnabled() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeGetActivatingInfo(
    JNIEnv * env, jclass clazz)
{
  if (!g_framework) return jni::ToJavaString(env, "");
  return jni::ToJavaString(env, g_framework->NativeFramework()->GetIndoorManager().GetActivatingInfo());
}

// screenX, screenY are device-pixel coordinates (from MotionEvent).
JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_maplayer_indoor_IndoorManager_nativeGetDebugFeatureAt(
    JNIEnv * env, jclass clazz, jint screenX, jint screenY)
{
  if (!g_framework) return jni::ToJavaString(env, "");
  auto const * fw = g_framework->NativeFramework();
  m2::PointD const mercPt = fw->PtoG(m2::PointD(screenX, screenY));
  return jni::ToJavaString(env, fw->GetDebugIndoorFeature(mercPt));
}
}
