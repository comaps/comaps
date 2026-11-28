#include <jni.h>

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"

namespace
{
using namespace osm;
using namespace jni;

bool LoadOsmUserPreferences(std::string const & oauthToken, UserPreferences & outPrefs)
{
  ServerApi06 const api(OsmOAuth::ServerAuth(oauthToken));
  outPrefs = api.GetUserPreferences();
  return (outPrefs.m_id != 0);
}
}  // namespace

extern "C"
{
JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOAuth2Url(JNIEnv * env, jclass)
{
  auto const auth = OsmOAuth::ServerAuth();
  return ToJavaString(env, auth.BuildOAuth2Url());
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeBuildOAuth2Url(JNIEnv * env, jclass,
                                                                                        jstring state,
                                                                                        jstring codeChallenge)
{
  auto const auth = OsmOAuth::ServerAuth();
  return ToJavaString(env, auth.BuildOAuth2Url(ToNativeString(env, state), ToNativeString(env, codeChallenge)));
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeAuthWithOAuth2Code(JNIEnv * env, jclass,
                                                                                            jstring oauth2code,
                                                                                            jstring codeVerifier)
{
  OsmOAuth auth = OsmOAuth::ServerAuth();
  try
  {
    auto token = auth.FinishAuthorization(ToNativeString(env, oauth2code), ToNativeString(env, codeVerifier));
    if (!token.empty())
    {
      auth.SetAuthToken(token);
      return ToJavaString(env, token);
    }
    LOG(LWARNING, ("nativeAuthWithOAuth2Code: invalid OAuth2 code", oauth2code));
  }
  catch (std::exception const & ex)
  {
    LOG(LWARNING, ("nativeAuthWithOAuth2Code error ", ex.what()));
  }
  return nullptr;
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOsmUsername(JNIEnv * env, jclass,
                                                                                        jstring oauthToken)
{
  UserPreferences prefs;
  if (LoadOsmUserPreferences(jni::ToNativeString(env, oauthToken), prefs))
    return jni::ToJavaString(env, prefs.m_displayName);
  return nullptr;
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOsmChangesetsCount(JNIEnv * env, jclass,
                                                                                            jstring oauthToken)
{
  UserPreferences prefs;
  if (LoadOsmUserPreferences(jni::ToNativeString(env, oauthToken), prefs))
    return prefs.m_changesets;
  return -1;
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetOsmProfilePictureUrl(JNIEnv * env, jclass,
                                                                                                 jstring oauthToken)
{
  UserPreferences prefs;
  if (LoadOsmUserPreferences(jni::ToNativeString(env, oauthToken), prefs))
    return jni::ToJavaString(env, prefs.m_imageUrl);
  return nullptr;
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetHistoryUrl(JNIEnv * env, jclass,
                                                                                       jstring user)
{
  return jni::ToJavaString(env, OsmOAuth::ServerAuth().GetHistoryURL(jni::ToNativeString(env, user)));
}

JNIEXPORT jstring JNICALL Java_app_organicmaps_sdk_editor_OsmOAuth_nativeGetNotesUrl(JNIEnv * env, jclass, jstring user)
{
  return jni::ToJavaString(env, OsmOAuth::ServerAuth().GetNotesURL(jni::ToNativeString(env, user)));
}
}  // extern "C"
