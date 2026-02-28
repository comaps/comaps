#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_MapAppearance_nativeSet(JNIEnv *, jclass, jint mapAppearance)
{
  auto const val = static_cast<MapAppearance>(mapAppearance);
  if (val != g_framework->CurrentMapAppearance())
    g_framework->SwitchToMapAppearance(val);
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_MapAppearance_nativeGet(JNIEnv *, jclass)
{
  return g_framework->CurrentMapAppearance();
}
}
