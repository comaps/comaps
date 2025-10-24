package app.organicmaps.location;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;

import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RoutingInfo;

import java.util.Locale;

/**
 * Helper for creating and updating location sharing notifications.
 */
public class LocationSharingNotification
{
  public static final String CHANNEL_ID = "LOCATION_SHARING";
  private static final String CHANNEL_NAME = "Live Location Sharing";

  private final Context mContext;
  private final NotificationManagerCompat mNotificationManager;

  public LocationSharingNotification(@NonNull Context context)
  {
    mContext = context;
    mNotificationManager = NotificationManagerCompat.from(context);
    createNotificationChannel();
  }

  private void createNotificationChannel()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)
      return;

    NotificationChannel channel = new NotificationChannel(
        CHANNEL_ID,
        CHANNEL_NAME,
        NotificationManager.IMPORTANCE_LOW); // Low importance = no sound/vibration

    channel.setDescription("Notifications for active live location sharing");
    channel.setShowBadge(false);
    channel.enableLights(false);
    channel.enableVibration(false);

    NotificationManager nm = mContext.getSystemService(NotificationManager.class);
    if (nm != null)
      nm.createNotificationChannel(channel);
  }

  /**
   * Build notification for location sharing service.
   * @param stopIntent PendingIntent to stop sharing
   * @return Notification object
   */
  @NonNull
  public Notification buildNotification(@NonNull PendingIntent stopIntent)
  {
    return buildNotification(stopIntent, null);
  }

  /**
   * Build notification with copy URL action.
   * @param stopIntent PendingIntent to stop sharing
   * @param copyUrlIntent PendingIntent to copy URL (optional)
   * @return Notification object
   */
  @NonNull
  public Notification buildNotification(
      @NonNull PendingIntent stopIntent,
      @Nullable PendingIntent copyUrlIntent)
  {
    Intent notificationIntent = new Intent(mContext, MwmActivity.class);
    PendingIntent pendingIntent = PendingIntent.getActivity(
        mContext,
        0,
        notificationIntent,
        PendingIntent.FLAG_IMMUTABLE);

    NotificationCompat.Builder builder = new NotificationCompat.Builder(mContext, CHANNEL_ID)
        .setSmallIcon(R.drawable.ic_share)
        .setContentIntent(pendingIntent)
        .setOngoing(true)
        .setPriority(NotificationCompat.PRIORITY_LOW)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setShowWhen(false)
        .setAutoCancel(false);

    // Title
    builder.setContentTitle(mContext.getString(R.string.location_sharing_active));

    // No subtitle - keep it simple

    // Copy URL action button (if provided)
    if (copyUrlIntent != null)
    {
      builder.addAction(
          R.drawable.ic_share,
          mContext.getString(R.string.location_sharing_copy_url),
          copyUrlIntent);
    }

    // Stop action button
    builder.addAction(
        R.drawable.ic_close,
        mContext.getString(R.string.location_sharing_stop),
        stopIntent);

    // Set foreground service type for Android 10+
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
    {
      builder.setForegroundServiceBehavior(NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE);
    }

    return builder.build();
  }

  /**
   * Update existing notification.
   * @param notificationId Notification ID
   * @param notification Updated notification
   */
  public void updateNotification(int notificationId, @NonNull Notification notification)
  {
    mNotificationManager.notify(notificationId, notification);
  }

  /**
   * Cancel notification.
   * @param notificationId Notification ID
   */
  public void cancelNotification(int notificationId)
  {
    mNotificationManager.cancel(notificationId);
  }
}
