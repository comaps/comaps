#include "Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "storage/storage_defines.hpp"

#include "base/logging.hpp"

#include "platform/settings.hpp"

namespace
{
void OnRenderingInitializationFinished(std::shared_ptr<jobject> const & listener)
{
  JNIEnv * env = jni::GetEnv();
  env->CallVoidMethod(*listener, jni::GetMethodID(env, *listener.get(), "onRenderingInitializationFinished", "()V"));
}
void AccessibilityUpdateCallback(std::shared_ptr<jobject> const & callback,
                                 std::vector<dp::TAccessibilityStableID> removed,
                                 std::vector<dp::TAccessibilityStableID> updated,
                                 std::vector<dp::TAccessibilityStableID> added)
{
  JNIEnv * env = jni::GetEnv();

  jintArray removedArray = env->NewIntArray(removed.size());
  jintArray updatedArray = env->NewIntArray(updated.size());
  jintArray addedArray = env->NewIntArray(added.size());

  if (!removed.empty())
    env->SetIntArrayRegion(removedArray, 0, removed.size(), removed.data());
  if (!updated.empty())
    env->SetIntArrayRegion(updatedArray, 0, updated.size(), updated.data());
  if (!added.empty())
    env->SetIntArrayRegion(addedArray, 0, added.size(), added.data());

  env->CallVoidMethod(*callback, jni::GetMethodID(env, *callback.get(), "frame", "([I[I[I)V"), removedArray,
                      updatedArray, addedArray);
}
}  // namespace

extern "C"
{
JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_Map_nativeCreateEngine(JNIEnv * env, jclass, jobject surface,
                                                                           jint density, jboolean firstLaunch,
                                                                           jboolean isLaunchByDeepLink,
                                                                           jint appVersionCode, jboolean isCustomROM)
{
  return g_framework->CreateDrapeEngine(env, surface, density, firstLaunch, isLaunchByDeepLink,
                                        base::asserted_cast<uint32_t>(appVersionCode), isCustomROM);
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_Map_nativeIsEngineCreated(JNIEnv *, jclass)
{
  return g_framework->IsDrapeEngineCreated();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeUpdateEngineDpi(JNIEnv *, jclass, jint dpi)
{
  return g_framework->UpdateDpi(dpi);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeExecuteMapApiRequest(JNIEnv * env, jclass)
{
  return g_framework->ExecuteMapApiRequest();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeSetRenderingInitializationFinishedListener(JNIEnv *, jclass,
                                                                                                     jobject listener)
{
  if (listener)
  {
    g_framework->NativeFramework()->SetGraphicsContextInitializationHandler(
        std::bind(&OnRenderingInitializationFinished, jni::make_global_ref(listener)));
  }
  else
  {
    g_framework->NativeFramework()->SetGraphicsContextInitializationHandler(nullptr);
  }
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_Map_nativeAttachSurface(JNIEnv * env, jclass, jobject surface)
{
  return g_framework->AttachSurface(env, surface);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeDetachSurface(JNIEnv *, jclass, jboolean destroySurface)
{
  g_framework->DetachSurface(destroySurface);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeSurfaceChanged(JNIEnv * env, jclass, jobject surface, jint w,
                                                                         jint h)
{
  g_framework->Resize(env, surface, w, h);
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_Map_nativeDestroySurfaceOnDetach(JNIEnv *, jclass)
{
  return g_framework->DestroySurfaceOnDetach();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativePauseSurfaceRendering(JNIEnv *, jclass)
{
  g_framework->PauseSurfaceRendering();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeResumeSurfaceRendering(JNIEnv *, jclass)
{
  g_framework->ResumeSurfaceRendering();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeUpdateMyPositionRoutingOffset(JNIEnv * env, jclass clazz,
                                                                                        int offsetY)
{
  g_framework->UpdateMyPositionRoutingOffset(offsetY);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeApplyWidgets(JNIEnv *, jclass)
{
  g_framework->ApplyWidgets();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeCleanWidgets(JNIEnv *, jclass)
{
  g_framework->CleanWidgets();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeSetupWidget(JNIEnv *, jclass, jint widget, jfloat x, jfloat y,
                                                                      jint anchor)
{
  g_framework->SetupWidget(static_cast<gui::EWidget>(widget), x, y, static_cast<dp::Anchor>(anchor));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeCompassUpdated(JNIEnv *, jclass, jdouble north,
                                                                         jboolean forceRedraw)
{
  location::CompassInfo info;
  info.m_bearing = north;

  g_framework->OnCompassUpdated(info, forceRedraw);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeScalePlus(JNIEnv *, jclass)
{
  g_framework->Scale(::Framework::SCALE_MAG);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeScaleMinus(JNIEnv *, jclass)
{
  g_framework->Scale(::Framework::SCALE_MIN);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeOnScroll(JNIEnv *, jclass, jdouble distanceX,
                                                                   jdouble distanceY)
{
  g_framework->Scroll(distanceX, distanceY);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeOnScale(JNIEnv *, jclass, jdouble factor, jdouble focusX,
                                                                  jdouble focusY, jboolean isAnim)
{
  g_framework->Scale(factor, {focusX, focusY}, isAnim);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeOnTouch(JNIEnv *, jclass, jint action, jint id1, jfloat x1,
                                                                  jfloat y1, jint id2, jfloat x2, jfloat y2,
                                                                  jint maskedPointer)
{
  g_framework->Touch(action, android::Framework::Finger(id1, x1, y1), android::Framework::Finger(id2, x2, y2),
                     maskedPointer);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeStorageConnected(JNIEnv *, jclass)
{
  android::Platform::Instance().OnExternalStorageStatusChanged(true);
  g_framework->AddLocalMaps();
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Map_nativeStorageDisconnected(JNIEnv *, jclass)
{
  android::Platform::Instance().OnExternalStorageStatusChanged(false);
  g_framework->RemoveLocalMaps();
}

void fillAccessibilityNodeContext(JNIEnv * env, dp::AccessibilityNodeContext const & info, jclass const resultClass,
                                  jobject const result)
{
  jni::TScopedLocalRef const accessibilityLabelString(env,
                                                      jni::ToJavaString(env, info.GetNodeInfo().m_accessibilityLabel));

  static jfieldID const topField = env->GetFieldID(resultClass, "top", "D");
  ASSERT(topField, ());
  env->SetDoubleField(result, topField, info.GetBounds().minY());

  static jfieldID const leftField = env->GetFieldID(resultClass, "left", "D");
  ASSERT(leftField, ());
  env->SetDoubleField(result, leftField, info.GetBounds().minX());

  static jfieldID const bottomField = env->GetFieldID(resultClass, "bottom", "D");
  ASSERT(bottomField, ());
  env->SetDoubleField(result, bottomField, info.GetBounds().maxY());

  static jfieldID const rightField = env->GetFieldID(resultClass, "right", "D");
  ASSERT(rightField, ());
  env->SetDoubleField(result, rightField, info.GetBounds().maxX());

  static jfieldID const accessibilityLabelField =
      env->GetFieldID(resultClass, "accessibilityLabel", "Ljava/lang/String;");
  ASSERT(accessibilityLabelField, ());
  env->SetObjectField(result, accessibilityLabelField, accessibilityLabelString);

  static jfieldID const explorationTypeField = env->GetFieldID(resultClass, "explorationType", "I");
  ASSERT(explorationTypeField, ());
  env->SetIntField(result, explorationTypeField, info.GetNodeInfo().m_explorationType);
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_Map_nativeGetAccessibilityNode(JNIEnv * env, jclass, jint id,
                                                                                   jobject result)
{
  auto info = g_framework->GetAccessibilityNodeContext(id);

  if (!info)
    return false;

  static jclass const resultClass = env->GetObjectClass(result);
  ASSERT(resultClass, ());

  fillAccessibilityNodeContext(env, *info->get(), resultClass, result);

  return true;
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_Map_nativeGetAccessibilityNodeAtPoint(JNIEnv * env, jclass, jfloat x,
                                                                                      jfloat y)
{
  auto id = g_framework->GetAccessibilityNodeAtPoint(m2::PointD(x, y));
  if (!id)
    return 0;

  return id;
}

static_assert(std::numeric_limits<dp::TAccessibilityStableID>::max() == std::numeric_limits<jint>::max());

JNIEXPORT jobject JNICALL Java_app_organicmaps_sdk_Map_nativeGetAllAccessibilityNodes(JNIEnv * env, jclass)
{
  auto ids = g_framework->GetAllAccessibilityNodes();

  jintArray array = env->NewIntArray(ids.size());

  if (!ids.empty())
    env->SetIntArrayRegion(array, 0, ids.size(), ids.data());

  return array;
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_Map_nativeSetAccessibilityUpdateCallback(JNIEnv * env, jclass,
                                                                                             jobject callback)
{
  if (callback)
  {
    return g_framework->SetAccessibilityUpdateCallback(
        std::bind(&AccessibilityUpdateCallback, jni::make_global_ref(callback), _1, _2, _3));
  }
  g_framework->SetAccessibilityUpdateCallback({});
  return true;
}
}  // extern "C"
