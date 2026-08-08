package app.organicmaps.sdk.editor;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;
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
  public static native String nativeAuthWithOAuth2Code(String oauth2code);

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
}
