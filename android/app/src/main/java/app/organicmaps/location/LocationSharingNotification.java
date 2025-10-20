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
    return buildNotification(stopIntent, null, null);
  }

  /**
   * Build notification with current location and routing info.
   * @param stopIntent PendingIntent to stop sharing
   * @param location Current location (optional)
   * @param routingInfo Navigation info (optional)
   * @return Notification object
   */
  @NonNull
  public Notification buildNotification(
      @NonNull PendingIntent stopIntent,
      @Nullable Location location,
      @Nullable RoutingInfo routingInfo)
  {
    Intent notificationIntent = new Intent(mContext, MwmActivity.class);
    PendingIntent pendingIntent = PendingIntent.getActivity(
        mContext,
        0,
        notificationIntent,
        PendingIntent.FLAG_IMMUTABLE);

    NotificationCompat.Builder builder = new NotificationCompat.Builder(mContext, CHANNEL_ID)
        .setSmallIcon(R.drawable.ic_location_sharing)
        .setContentIntent(pendingIntent)
        .setOngoing(true)
        .setPriority(NotificationCompat.PRIORITY_LOW)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setShowWhen(false)
        .setAutoCancel(false);

    // Title
    builder.setContentTitle(mContext.getString(R.string.location_sharing_active));

    // Content text
    String contentText = buildContentText(location, routingInfo);
    builder.setContentText(contentText);

    // Big text style for more details
    if (routingInfo != null)
    {
      NotificationCompat.BigTextStyle bigTextStyle = new NotificationCompat.BigTextStyle()
          .bigText(contentText)
          .setSummaryText(mContext.getString(R.string.location_sharing_tap_to_view));
      builder.setStyle(bigTextStyle);
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

  @NonNull
  private String buildContentText(@Nullable Location location, @Nullable RoutingInfo routingInfo)
  {
    StringBuilder text = new StringBuilder();

    // If navigating, show ETA and distance
    if (routingInfo != null && routingInfo.distToTarget != null)
    {
      if (routingInfo.totalTimeInSeconds > 0)
      {
        String eta = formatTime(routingInfo.totalTimeInSeconds);
        text.append(mContext.getString(R.string.location_sharing_eta, eta));
      }

      if (routingInfo.distToTarget != null && routingInfo.distToTarget.isValid())
      {
        if (text.length() > 0)
          text.append(" • ");
        text.append(routingInfo.distToTarget.toString(mContext));
        text.append(" ").append(mContext.getString(R.string.location_sharing_remaining));
      }
    }
    else
    {
      // Standalone mode - show accuracy if available
      if (location != null)
      {
        text.append(mContext.getString(R.string.location_sharing_accuracy,
            formatAccuracy(location.getAccuracy())));
      }
      else
      {
        text.append(mContext.getString(R.string.location_sharing_waiting_for_location));
      }
    }

    return text.toString();
  }

  @NonNull
  private String formatTime(int seconds)
  {
    if (seconds < 60)
      return String.format(Locale.US, "%ds", seconds);

    int minutes = seconds / 60;
    if (minutes < 60)
      return String.format(Locale.US, "%d min", minutes);

    int hours = minutes / 60;
    int remainingMinutes = minutes % 60;
    return String.format(Locale.US, "%dh %dm", hours, remainingMinutes);
  }

  @NonNull
  private String formatAccuracy(float accuracyMeters)
  {
    if (accuracyMeters < 10)
      return mContext.getString(R.string.location_sharing_accuracy_high);
    else if (accuracyMeters < 50)
      return mContext.getString(R.string.location_sharing_accuracy_medium);
    else
      return mContext.getString(R.string.location_sharing_accuracy_low);
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
