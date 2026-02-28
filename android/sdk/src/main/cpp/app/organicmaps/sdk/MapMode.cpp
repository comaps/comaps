#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_MapMode_nativeSet(JNIEnv *, jclass, jint mapMode)
{
  auto const val = static_cast<MapMode>(mapMode);
  if (val != g_framework->CurrentMapMode())
    g_framework->SwitchToMapMode(val);
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_MapMode_nativeGet(JNIEnv *, jclass)
{
  return g_framework->CurrentMapMode();
}
}
