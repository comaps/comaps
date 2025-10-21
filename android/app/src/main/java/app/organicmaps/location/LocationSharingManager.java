package app.organicmaps.location;

import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;

/**
 * Singleton manager for live location sharing functionality.
 * Coordinates between LocationHelper, RoutingController, and LocationSharingService.
 */
public class LocationSharingManager
{
  private static final String TAG = LocationSharingManager.class.getSimpleName();

  private static LocationSharingManager sInstance;

  @Nullable
  private String mSessionId;
  @Nullable
  private String mEncryptionKey;
  @Nullable
  private String mShareUrl;
  private boolean mIsSharing = false;

  private final Context mContext;

  private LocationSharingManager()
  {
    mContext = MwmApplication.sInstance;
  }

  @NonNull
  public static synchronized LocationSharingManager getInstance()
  {
    if (sInstance == null)
      sInstance = new LocationSharingManager();
    return sInstance;
  }

  /**
   * Start live location sharing.
   * @return Share URL that can be sent to others
   */
  @Nullable
  public String startSharing()
  {
    if (mIsSharing)
    {
      Logger.w(TAG, "Location sharing already active");
      return mShareUrl;
    }

    // Generate session credentials via native code
    String[] credentials = nativeGenerateSessionCredentials();
    if (credentials == null || credentials.length != 2)
    {
      Logger.e(TAG, "Failed to generate session credentials");
      return null;
    }

    mSessionId = credentials[0];
    mEncryptionKey = credentials[1];

    // Generate share URL using configured server
    String serverUrl = Config.LocationSharing.getServerUrl();
    mShareUrl = nativeGenerateShareUrl(mSessionId, mEncryptionKey, serverUrl);
    if (mShareUrl == null)
    {
      Logger.e(TAG, "Failed to generate share URL");
      return null;
    }

    mIsSharing = true;

    // Start foreground service
    Intent intent = new Intent(mContext, LocationSharingService.class);
    intent.putExtra(LocationSharingService.EXTRA_SESSION_ID, mSessionId);
    intent.putExtra(LocationSharingService.EXTRA_ENCRYPTION_KEY, mEncryptionKey);
    intent.putExtra(LocationSharingService.EXTRA_SERVER_URL, serverUrl);
    intent.putExtra(LocationSharingService.EXTRA_UPDATE_INTERVAL, Config.LocationSharing.getUpdateInterval());

    mContext.startForegroundService(intent);

    Logger.i(TAG, "Location sharing started, session ID: " + mSessionId);

    return mShareUrl;
  }

  /**
   * Stop live location sharing.
   */
  public void stopSharing()
  {
    if (!mIsSharing)
    {
      Logger.w(TAG, "Location sharing not active");
      return;
    }

    // Stop foreground service
    Intent intent = new Intent(mContext, LocationSharingService.class);
    mContext.stopService(intent);

    mIsSharing = false;
    mSessionId = null;
    mEncryptionKey = null;
    mShareUrl = null;

    Logger.i(TAG, "Location sharing stopped");
  }

  public boolean isSharing()
  {
    return mIsSharing;
  }

  @Nullable
  public String getShareUrl()
  {
    return mShareUrl;
  }

  @Nullable
  public String getSessionId()
  {
    return mSessionId;
  }

  public void setUpdateIntervalSeconds(int seconds)
  {
    Config.LocationSharing.setUpdateInterval(seconds);
  }

  public int getUpdateIntervalSeconds()
  {
    return Config.LocationSharing.getUpdateInterval();
  }

  public void setServerBaseUrl(@NonNull String url)
  {
    Config.LocationSharing.setServerUrl(url);
  }

  @NonNull
  public String getServerBaseUrl()
  {
    return Config.LocationSharing.getServerUrl();
  }

  /**
   * Get current battery level (0-100).
   */
  public int getBatteryLevel()
  {
    BatteryManager bm = (BatteryManager) mContext.getSystemService(Context.BATTERY_SERVICE);
    if (bm == null)
      return 100;

    return bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
  }

  /**
   * Check if currently navigating with an active route.
   */
  public boolean isNavigating()
  {
    return RoutingController.get().isNavigating();
  }

  // Native methods (implemented in JNI)

  /**
   * Generate new session credentials (ID and encryption key).
   * @return Array of [sessionId, encryptionKey]
   */
  @Nullable
  private static native String[] nativeGenerateSessionCredentials();

  /**
   * Generate shareable URL from credentials.
   * @param sessionId Session ID (UUID)
   * @param encryptionKey Base64-encoded encryption key
   * @param serverBaseUrl Server base URL
   * @return Share URL
   */
  @Nullable
  private static native String nativeGenerateShareUrl(String sessionId, String encryptionKey, String serverBaseUrl);

  /**
   * Encrypt location payload.
   * @param encryptionKey Base64-encoded encryption key
   * @param payloadJson JSON payload to encrypt
   * @return Encrypted payload JSON (with iv, ciphertext, authTag) or null on failure
   */
  @Nullable
  public static native String nativeEncryptPayload(String encryptionKey, String payloadJson);
}
