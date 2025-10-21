package app.organicmaps.location;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.location.Location;
import android.os.BatteryManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.api.LocationSharingApiClient;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.log.Logger;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Locale;

/**
 * Foreground service for live GPS location sharing.
 * Monitors location updates and posts encrypted data to server at regular intervals.
 */
public class LocationSharingService extends Service implements LocationListener
{
  private static final String TAG = LocationSharingService.class.getSimpleName();
  private static final int NOTIFICATION_ID = 0x1002; // Unique ID for location sharing

  // Intent extras
  public static final String EXTRA_SESSION_ID = "session_id";
  public static final String EXTRA_ENCRYPTION_KEY = "encryption_key";
  public static final String EXTRA_SERVER_URL = "server_url";
  public static final String EXTRA_UPDATE_INTERVAL = "update_interval";

  // Action for notification stop button
  private static final String ACTION_STOP = "app.organicmaps.ACTION_STOP_LOCATION_SHARING";

  @Nullable
  private String mSessionId;
  @Nullable
  private String mEncryptionKey;
  @Nullable
  private String mServerUrl;
  private int mUpdateIntervalSeconds = 20;

  @Nullable
  private Location mLastLocation;
  private long mLastUpdateTimestamp = 0;

  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private final Runnable mUpdateTask = this::processLocationUpdate;

  @Nullable
  private LocationSharingApiClient mApiClient;
  @Nullable
  private LocationSharingNotification mNotificationHelper;

  @Override
  public void onCreate()
  {
    super.onCreate();
    Logger.i(TAG, "Service created");

    mNotificationHelper = new LocationSharingNotification(this);
  }

  @Override
  public int onStartCommand(@Nullable Intent intent, int flags, int startId)
  {
    if (intent == null)
    {
      Logger.w(TAG, "Null intent, stopping service");
      stopSelf();
      return START_NOT_STICKY;
    }

    // Handle stop action from notification
    if (ACTION_STOP.equals(intent.getAction()))
    {
      Logger.i(TAG, "Stop action received from notification");
      LocationSharingManager.getInstance().stopSharing();
      stopSelf();
      return START_NOT_STICKY;
    }

    // Extract session info
    mSessionId = intent.getStringExtra(EXTRA_SESSION_ID);
    mEncryptionKey = intent.getStringExtra(EXTRA_ENCRYPTION_KEY);
    mServerUrl = intent.getStringExtra(EXTRA_SERVER_URL);
    mUpdateIntervalSeconds = intent.getIntExtra(EXTRA_UPDATE_INTERVAL, 20);

    if (mSessionId == null || mEncryptionKey == null || mServerUrl == null)
    {
      Logger.e(TAG, "Missing session info, stopping service");
      stopSelf();
      return START_NOT_STICKY;
    }

    // Initialize API client
    mApiClient = new LocationSharingApiClient(mServerUrl, mSessionId);

    // Create session on server
    mApiClient.createSession(new LocationSharingApiClient.Callback()
    {
      @Override
      public void onSuccess()
      {
        Logger.i(TAG, "Session created on server");
      }

      @Override
      public void onFailure(@NonNull String error)
      {
        Logger.w(TAG, "Failed to create session on server: " + error);
      }
    });

    // Start foreground with notification
    Notification notification = mNotificationHelper != null
        ? mNotificationHelper.buildNotification(getStopIntent())
        : buildFallbackNotification();

    startForeground(NOTIFICATION_ID, notification);

    // Register for location updates
    LocationHelper locationHelper = MwmApplication.sInstance.getLocationHelper();
    locationHelper.addListener(this);

    Logger.i(TAG, "Service started for session: " + mSessionId);

    return START_STICKY;
  }

  @Override
  public void onDestroy()
  {
    Logger.i(TAG, "Service destroyed");

    // Unregister location listener
    LocationHelper locationHelper = MwmApplication.sInstance.getLocationHelper();
    locationHelper.removeListener(this);

    // Cancel pending updates
    mHandler.removeCallbacks(mUpdateTask);

    // Send session end to server (optional)
    if (mApiClient != null && mSessionId != null)
      mApiClient.endSession();

    super.onDestroy();
  }

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null; // Not a bound service
  }

  // LocationHelper.LocationListener implementation

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    mLastLocation = location;

    // Update notification with location info
    if (mNotificationHelper != null)
    {
      Notification notification = mNotificationHelper.buildNotification(
          getStopIntent(),
          location,
          getNavigationInfo());
      mNotificationHelper.updateNotification(NOTIFICATION_ID, notification);
    }

    // Schedule update if needed
    scheduleUpdate();
  }


  // Private methods

  private void scheduleUpdate()
  {
    long now = System.currentTimeMillis();
    long timeSinceLastUpdate = (now - mLastUpdateTimestamp) / 1000; // Convert to seconds

    if (timeSinceLastUpdate >= mUpdateIntervalSeconds)
    {
      // Remove any pending updates
      mHandler.removeCallbacks(mUpdateTask);
      // Execute immediately
      mHandler.post(mUpdateTask);
    }
  }

  private void processLocationUpdate()
  {
    if (mLastLocation == null || mEncryptionKey == null || mApiClient == null)
      return;

    // Check battery level
    int batteryLevel = getBatteryLevel();
    if (batteryLevel < 10)
    {
      Logger.w(TAG, "Battery level too low (" + batteryLevel + "%), stopping sharing");
      LocationSharingManager.getInstance().stopSharing();
      stopSelf();
      return;
    }

    // Build payload JSON
    JSONObject payload = buildPayloadJson(mLastLocation, batteryLevel);
    if (payload == null)
      return;

    // Encrypt payload
    String encryptedJson = LocationCrypto.encrypt(mEncryptionKey, payload.toString());
    if (encryptedJson == null)
    {
      Logger.e(TAG, "Failed to encrypt payload");
      return;
    }

    // Send to server
    mApiClient.updateLocation(encryptedJson, new LocationSharingApiClient.Callback()
    {
      @Override
      public void onSuccess()
      {
        Logger.d(TAG, "Location update sent successfully");
        mLastUpdateTimestamp = System.currentTimeMillis();
      }

      @Override
      public void onFailure(@NonNull String error)
      {
        Logger.w(TAG, "Failed to send location update: " + error);
      }
    });
  }

  @Nullable
  private JSONObject buildPayloadJson(@NonNull Location location, int batteryLevel)
  {
    try
    {
      JSONObject json = new JSONObject();
      json.put("timestamp", System.currentTimeMillis() / 1000); // Unix timestamp
      json.put("lat", location.getLatitude());
      json.put("lon", location.getLongitude());
      json.put("accuracy", location.getAccuracy());

      if (location.hasSpeed())
        json.put("speed", location.getSpeed());

      if (location.hasBearing())
        json.put("bearing", location.getBearing());

      // Check if navigating
      RoutingInfo routingInfo = getNavigationInfo();
      if (routingInfo != null && routingInfo.distToTarget != null)
      {
        json.put("mode", "navigation");

        // Calculate ETA (current time + time remaining)
        if (routingInfo.totalTimeInSeconds > 0)
        {
          long etaTimestamp = (System.currentTimeMillis() / 1000) + routingInfo.totalTimeInSeconds;
          json.put("eta", etaTimestamp);
        }

        // Distance remaining in meters
        if (routingInfo.distToTarget != null)
        {
          json.put("distanceRemaining", routingInfo.distToTarget.mDistance);
        }
      }
      else
      {
        json.put("mode", "standalone");
      }

      json.put("batteryLevel", batteryLevel);

      return json;
    }
    catch (JSONException e)
    {
      Logger.e(TAG, "Failed to build payload JSON", e);
      return null;
    }
  }

  @Nullable
  private RoutingInfo getNavigationInfo()
  {
    if (!RoutingController.get().isNavigating())
      return null;

    return RoutingController.get().getCachedRoutingInfo();
  }

  private int getBatteryLevel()
  {
    BatteryManager bm = (BatteryManager) getSystemService(BATTERY_SERVICE);
    if (bm == null)
      return 100;

    return bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
  }

  @NonNull
  private PendingIntent getStopIntent()
  {
    Intent stopIntent = new Intent(this, LocationSharingService.class);
    stopIntent.setAction(ACTION_STOP);
    return PendingIntent.getService(this, 0, stopIntent,
        PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
  }

  @NonNull
  private Notification buildFallbackNotification()
  {
    Intent notificationIntent = new Intent(this, MwmActivity.class);
    PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent,
        PendingIntent.FLAG_IMMUTABLE);

    return new NotificationCompat.Builder(this, LocationSharingNotification.CHANNEL_ID)
        .setContentTitle(getString(R.string.location_sharing_active))
        .setContentText(getString(R.string.location_sharing_notification_text))
        .setSmallIcon(R.drawable.ic_location_sharing)
        .setContentIntent(pendingIntent)
        .setOngoing(true)
        .build();
  }
}
