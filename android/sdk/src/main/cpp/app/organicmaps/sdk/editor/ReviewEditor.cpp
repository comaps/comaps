#include <jni.h>

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "base/logging.hpp"

#include "editor/review.hpp"

extern "C"
{
JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_ReviewEditor_nativeGetReviewEditorUrl(JNIEnv * env, jclass, jobject featureId)
{
  auto const & fid = g_framework->BuildFeatureId(env, featureId);
  LOG(LINFO, ("Resolving review editor URL for", fid));
  auto const mo = g_framework->GetMapObjectByID(fid);
  auto const url = reviews::GetReviewEditorUrl(mo);
  LOG(LINFO, ("Resolved Editor URL:", url));
  return url.transform([&env](std::string const & s) { return jni::ToJavaString(env, s); }).value_or(nullptr);
}
}  // extern "C"
