package app.organicmaps.sdk.editor;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;
import android.util.Base64;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;
import androidx.annotation.WorkerThread;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;

public final class OsmOAuth
{
  private OsmOAuth() {}

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private static SharedPreferences mPrefs;
  private static Context mContext;
  private static final String TAG = OsmOAuth.class.getSimpleName();
  private static final String PREF_OSM_USERNAME = "OsmUsername";
  private static final String PREF_OSM_CHANGESETS_COUNT = "OsmChangesetsCount";
  private static final String PREF_OSM_OAUTH2_TOKEN = "OsmOAuth2Token";
  private static final String PREF_OSM_PROFILE_PICTURE_URL = "OsmProfilePictureUrl";
  private static final String PREF_OSM_OAUTH2_STATE = "OsmOAuth2State";
  private static final String PREF_OSM_OAUTH2_CODE_VERIFIER = "OsmOAuth2CodeVerifier";
  private static final int OAUTH2_RANDOM_BYTES = 32;
  private static final int BASE64_URL_FLAGS = Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP;
  private static final SecureRandom sSecureRandom = new SecureRandom();
  private static final String PROFILE_PICTURE_FILENAME = "osm_profile_picture.png";

  public static final String URL_PARAM_VERIFIER = "oauth_verifier";

  public static void init(@NonNull Context context, @NonNull SharedPreferences prefs)
  {
    mContext = context.getApplicationContext();
    mPrefs = prefs;
  }

  public static boolean isAuthorized()
  {
    return mPrefs.contains(PREF_OSM_OAUTH2_TOKEN);
  }

  public static String getAuthToken()
  {
    return mPrefs.getString(PREF_OSM_OAUTH2_TOKEN, "");
  }

  public static String getUsername()
  {
    return mPrefs.getString(PREF_OSM_USERNAME, "");
  }

  @NonNull
  private static File getProfilePictureCacheFile()
  {
    return new File(StorageUtils.getTempPath(mContext), PROFILE_PICTURE_FILENAME);
  }

  @WorkerThread
  @Nullable
  public static Bitmap getProfilePicture()
  {
    final String token = getAuthToken();
    final String pictureUrl = nativeGetOsmProfilePictureUrl(token);
    if (TextUtils.isEmpty(pictureUrl))
    {
      clearCachedProfilePicture();
      return null;
    }

    final String cachedUrl = mPrefs.getString(PREF_OSM_PROFILE_PICTURE_URL, "");
    final File cacheFile = getProfilePictureCacheFile();

    if (!pictureUrl.equals(cachedUrl) || !cacheFile.exists())
    {
      final Bitmap bitmap = fetchBitmapFromUrl(pictureUrl);
      if (bitmap != null)
      {
        saveBitmapToCache(bitmap, cacheFile);
        mPrefs.edit().putString(PREF_OSM_PROFILE_PICTURE_URL, pictureUrl).apply();
        return bitmap;
      }
    }

    return loadCachedProfilePicture();
  }

  private static void clearCachedProfilePicture()
  {
    getProfilePictureCacheFile().delete();
    mPrefs.edit().remove(PREF_OSM_PROFILE_PICTURE_URL).apply();
  }

  @Nullable
  private static Bitmap loadCachedProfilePicture()
  {
    final File cacheFile = getProfilePictureCacheFile();
    if (!cacheFile.exists())
      return null;
    return BitmapFactory.decodeFile(cacheFile.getAbsolutePath());
  }

  @Nullable
  private static Bitmap fetchBitmapFromUrl(@NonNull String url)
  {
    try
    {
      HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
      connection.setConnectTimeout(Constants.CONNECT_TIMEOUT_MS);
      connection.setReadTimeout(Constants.READ_TIMEOUT_MS);
      try (InputStream in = connection.getInputStream())
      {
        return BitmapFactory.decodeStream(in);
      }
      finally
      {
        connection.disconnect();
      }
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Failed to fetch profile picture", e);
      return null;
    }
  }

  private static void saveBitmapToCache(@NonNull Bitmap bitmap, @NonNull File file)
  {
    try (FileOutputStream out = new FileOutputStream(file))
    {
      bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Failed to cache profile picture", e);
    }
  }

  public static void setAuthorization(String oauthToken, String username)
  {
    mPrefs.edit().putString(PREF_OSM_OAUTH2_TOKEN, oauthToken).putString(PREF_OSM_USERNAME, username).apply();
  }

  @NonNull
  public static String startOAuth2Flow()
  {
    final String state = generateRandomUrlSafeString();
    final String codeVerifier = generateRandomUrlSafeString();
    mPrefs.edit()
        .putString(PREF_OSM_OAUTH2_STATE, state)
        .putString(PREF_OSM_OAUTH2_CODE_VERIFIER, codeVerifier)
        .apply();
    return nativeBuildOAuth2Url(state, computeCodeChallenge(codeVerifier));
  }

  @Nullable
  public static String consumeOAuth2CodeVerifier(@Nullable String state)
  {
    final String expectedState = mPrefs.getString(PREF_OSM_OAUTH2_STATE, null);
    final String codeVerifier = mPrefs.getString(PREF_OSM_OAUTH2_CODE_VERIFIER, null);
    if (state == null || expectedState == null || codeVerifier == null
        || !MessageDigest.isEqual(expectedState.getBytes(StandardCharsets.US_ASCII),
                                  state.getBytes(StandardCharsets.US_ASCII)))
      return null;

    mPrefs.edit().remove(PREF_OSM_OAUTH2_STATE).remove(PREF_OSM_OAUTH2_CODE_VERIFIER).apply();
    return codeVerifier;
  }

  @NonNull
  private static String generateRandomUrlSafeString()
  {
    final byte[] bytes = new byte[OAUTH2_RANDOM_BYTES];
    sSecureRandom.nextBytes(bytes);
    return Base64.encodeToString(bytes, BASE64_URL_FLAGS);
  }

  @NonNull
  private static String computeCodeChallenge(@NonNull String codeVerifier)
  {
    try
    {
      final byte[] digest =
          MessageDigest.getInstance("SHA-256").digest(codeVerifier.getBytes(StandardCharsets.US_ASCII));
      return Base64.encodeToString(digest, BASE64_URL_FLAGS);
    }
    catch (NoSuchAlgorithmException e)
    {
      throw new IllegalStateException("SHA-256 is not available", e);
    }
  }

  public static void clearAuthorization()
  {
    mPrefs.edit()
            .remove(PREF_OSM_USERNAME)
            .remove(PREF_OSM_OAUTH2_TOKEN)
            .remove(PREF_OSM_PROFILE_PICTURE_URL)
            .apply();
    getProfilePictureCacheFile().delete();
  }

  @NonNull
  public static String getHistoryUrl()
  {
    return nativeGetHistoryUrl(getUsername());
  }

  @NonNull
  public static String getNotesUrl()
  {
    return nativeGetNotesUrl(getUsername());
  }

  /*
   Returns 5 strings: ServerURL, ClientId, ClientSecret, Scope, RedirectUri
   */
  @NonNull
  public static native String nativeGetOAuth2Url();

  @NonNull
  private static native String nativeBuildOAuth2Url(@NonNull String state, @NonNull String codeChallenge);

  /**
   * @return string with OAuth2 token
   */
  @WorkerThread
  @Size(2)
  @Nullable
  public static native String nativeAuthWithPassword(String login, String password);

  /**
   * @return string with OAuth2 token
   */
  @WorkerThread
  @Nullable
  public static native String nativeAuthWithOAuth2Code(String oauth2code, String codeVerifier);

  @WorkerThread
  @Nullable
  public static native String nativeGetOsmUsername(String oauthToken);

  @WorkerThread
  @Nullable
  public static native String nativeGetOsmProfilePictureUrl(String oauthToken);

  @WorkerThread
  @NonNull
  public static native String nativeGetHistoryUrl(String user);

  @WorkerThread
  @NonNull
  public static native String nativeGetNotesUrl(String user);

  /**
   * @return < 0 if failed to get changesets count.
   */
  @WorkerThread
  private static native int nativeGetOsmChangesetsCount(String oauthToken);

  @WorkerThread
  public static int getOsmChangesetsCount(@NonNull NetworkPolicy.DialogPresenter dialogPresenter,
                                          @NonNull FragmentManager fm)
  {
    final int[] editsCount = {-1};
    NetworkPolicy.checkNetworkPolicy(dialogPresenter, fm, policy -> {
      if (!policy.canUseNetwork())
        return;

      final String token = getAuthToken();
      editsCount[0] = OsmOAuth.nativeGetOsmChangesetsCount(token);
    });
    if (editsCount[0] < 0)
      return mPrefs.getInt(PREF_OSM_CHANGESETS_COUNT, 0);

    mPrefs.edit().putInt(PREF_OSM_CHANGESETS_COUNT, editsCount[0]).apply();
    return editsCount[0];
  }

  public static boolean isDev()
  {
    return nativeGetOAuth2Url().contains("dev.openstreetmap.org");
  }

}
