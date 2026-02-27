package app.organicmaps.downloader;

import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.log.Logger;
import java.util.Objects;

public class DownloaderNotifier
{
  private static final String TAG = DownloaderNotifier.class.getSimpleName();

  private static final String CHANNEL_ID = "downloader";
  public static final int NOTIFICATION_ID = 1;

  private final Context mContext;
  private NotificationCompat.Builder mProgressNotificationBuilder = null;
  private String mNotificationCountryId = null;

  public DownloaderNotifier(Context context)
  {
    mContext = context;
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(context);
    final NotificationChannelCompat channel =
        new NotificationChannelCompat.Builder(CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
            .setName(context.getString(R.string.notification_channel_downloader))
            .setShowBadge(true)
            .setVibrationEnabled(false)
            .setLightsEnabled(false)
            .setSound(null, null)
            .build();
    notificationManager.createNotificationChannel(channel);
  }

  public void notifyDownloadFailed(@Nullable String countryId)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ContextCompat.checkSelfPermission(mContext, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
    {
      Logger.w(TAG, "Permission POST_NOTIFICATIONS is not granted, skipping notification");
      return;
    }

    final String countryName = MapManager.nativeGetName(countryId);
    final String title = mContext.getString(R.string.download_country_failed, countryName);

    final Notification notification = new NotificationCompat.Builder(mContext, CHANNEL_ID)
                                          .setAutoCancel(true)
                                          .setCategory(NotificationCompat.CATEGORY_ERROR)
                                          .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
                                          .setSmallIcon(R.drawable.ic_logo_small)
                                          .setColor(ContextCompat.getColor(mContext, R.color.notification))
                                          .setContentTitle(title)
                                          .setShowWhen(true)
                                          .setTicker(title)
                                          .setContentIntent(getNotificationPendingIntent(countryId))
                                          .setOnlyAlertOnce(true)
                                          .build();

    Logger.i(TAG, "Notifying about failed map download");
    final NotificationManagerCompat notificationManager = NotificationManagerCompat.from(mContext);
    notificationManager.notify(NOTIFICATION_ID, notification);
  }

  public void notifyProgress()
  {
    notifyProgress(null, 0, 0, 0, 0);
  }

  public void notifyProgress(@Nullable String countryId, int maxProgress, int progress, int numRegions, int numSubregions)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
        && ContextCompat.checkSelfPermission(mContext, POST_NOTIFICATIONS) != PERMISSION_GRANTED)
    {
      Logger.w(TAG, "Permission POST_NOTIFICATIONS is not granted, skipping notification");
      return;
    }

    // countries do not get notifications
    if (numSubregions > 1) {
      return;
    }

    NotificationManagerCompat.from(mContext).notify(NOTIFICATION_ID,
                                                    buildProgressNotification(countryId, maxProgress, progress, numRegions));
  }

  @NonNull
  public Notification buildProgressNotification()
  {
    return buildProgressNotification(null, 0, 0, 0);
  }

  @NonNull
  public Notification buildProgressNotification(@Nullable String countryId, int maxProgress, int progress, int numRegions)
  {
    var builder = getNotificationBuilder(countryId);

    builder.setProgress(maxProgress, progress, false);

    final String countryName = countryId != null ? MapManager.nativeGetName(countryId) : "";

    if(numRegions > 0){
      builder.setContentTitle(mContext.getString(R.string.downloader_downloading) + " " + countryName);
      builder.setContentText(mContext.getString(R.string.mb_downloaded, progress, maxProgress));
    }

    return builder.build();
  }

  @NonNull
  private NotificationCompat.Builder getNotificationBuilder(@Nullable String countryId)
  {
    if (mProgressNotificationBuilder == null || !Objects.equals(countryId, mNotificationCountryId))
    {
      mNotificationCountryId = countryId;

      mProgressNotificationBuilder =
          new NotificationCompat.Builder(mContext, CHANNEL_ID)
              .setAutoCancel(true)
              .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
              .setSmallIcon(R.drawable.ic_logo_small)
              .setColor(ContextCompat.getColor(mContext, R.color.notification))
              .setShowWhen(true)
              .setContentIntent(getNotificationPendingIntent(countryId))
              .setSound(null);
    }
    return mProgressNotificationBuilder;
  }

  @NonNull
  private PendingIntent getNotificationPendingIntent(@Nullable String countryId)
  {
    /// @todo Zooming to the countryId when tapping on the notification?
    /// Shows very low zoom level, need z=9/10, I suppose ...
    final int FLAG_IMMUTABLE = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final Intent contentIntent = MwmActivity.createShowMapIntent(mContext, countryId);
    contentIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    return PendingIntent.getActivity(mContext, 0, contentIntent, PendingIntent.FLAG_UPDATE_CURRENT | FLAG_IMMUTABLE);
  }
}
