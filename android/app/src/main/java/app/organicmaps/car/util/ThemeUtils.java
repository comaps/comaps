package app.organicmaps.car.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.annotation.UiThread;
import androidx.car.app.CarContext;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.MapAppearance;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;

public final class ThemeUtils
{
  public enum ThemeMode
  {
    AUTO(R.string.auto, Config.UiTheme.AUTO),
    SUNRISE_SUNSET(R.string.car_theme_sunrise_sunset, "sunrise_sunset"),
    LIGHT(R.string.off, Config.UiTheme.DEFAULT),
    NIGHT(R.string.on, Config.UiTheme.NIGHT);

    ThemeMode(@StringRes int titleId, @NonNull String config)
    {
      mTitleId = titleId;
      mConfig = config;
    }

    @StringRes
    public int getTitleId()
    {
      return mTitleId;
    }

    @NonNull
    public String getConfig()
    {
      return mConfig;
    }

    @StringRes
    private final int mTitleId;
    @NonNull
    private final String mConfig;
  }

  private static final String ANDROID_AUTO_PREFERENCES_FILE_KEY = "ANDROID_AUTO_PREFERENCES_FILE_KEY";
  private static final String THEME_KEY = "ANDROID_AUTO_THEME_MODE";
  private static final long SUN_CHECK_INTERVAL_MS = 5 * 60 * 1000;

  @Nullable
  private static Runnable sSunCheck;

  @UiThread
  public static void update(@NonNull CarContext context)
  {
    final ThemeMode oldThemeMode = getThemeMode(context);
    update(context, oldThemeMode);
  }

  @UiThread
  public static void update(@NonNull CarContext context, @NonNull ThemeMode oldThemeMode)
  {
    final MapAppearance newMapAppearance = isNight(context, oldThemeMode) ? MapAppearance.Dark : MapAppearance.Light;

    if (MapAppearance.get() != newMapAppearance)
      MapAppearance.set(newMapAppearance);
    
    // Switch to car navigation style when using the car router (AA or car navigation on the phone).
    final boolean useVehicleStyle = RoutingController.get().isVehicleNavigation()
                                    || MwmApplication.from(context).getDisplayManager().isCarDisplayUsed();
    Framework.nativeSwitchToUsingVehicleStyle(useVehicleStyle);

    if (oldThemeMode == ThemeMode.SUNRISE_SUNSET)
      scheduleSunCheck(context);
    else
      cancelSunCheck();
  }

  public static boolean isNightMode(@NonNull CarContext context)
  {
    return isNight(context, getThemeMode(context));
  }

  @UiThread
  public static void cancelSunCheck()
  {
    if (sSunCheck == null)
      return;

    app.organicmaps.sdk.util.concurrency.UiThread.cancelDelayedTasks(sSunCheck);
    sSunCheck = null;
  }

  private static void scheduleSunCheck(@NonNull CarContext context)
  {
    cancelSunCheck();
    sSunCheck = () -> update(context);
    app.organicmaps.sdk.util.concurrency.UiThread.runLater(sSunCheck, SUN_CHECK_INTERVAL_MS);
  }

  private static boolean isNight(@NonNull CarContext context, @NonNull ThemeMode themeMode)
  {
    if (themeMode == ThemeMode.NIGHT)
      return true;
    if (themeMode == ThemeMode.SUNRISE_SUNSET)
      return !isDaytime(context);
    if (themeMode == ThemeMode.AUTO)
      return context.isDarkMode();
    return false;
  }

  private static boolean isDaytime(@NonNull CarContext context)
  {
    final Location last = MwmApplication.from(context).getLocationHelper().getSavedLocation();
    if (last == null)
      return !context.isDarkMode();

    final long currentTimeSec = System.currentTimeMillis() / 1000;
    return Framework.nativeIsDayTime(currentTimeSec, last.getLatitude(), last.getLongitude());
  }

  @SuppressLint("ApplySharedPref")
  @UiThread
  public static void setThemeMode(@NonNull CarContext context, @NonNull ThemeMode themeMode)
  {
    getSharedPreferences(context).edit().putString(THEME_KEY, themeMode.getConfig()).commit();
    update(context, themeMode);
  }

  @NonNull
  public static ThemeMode getThemeMode(@NonNull CarContext context)
  {
    final String themeMode = getSharedPreferences(context).getString(THEME_KEY, ThemeMode.AUTO.getConfig());

    for (final ThemeMode mode : ThemeMode.values())
    {
      if (themeMode.equals(mode.getConfig()))
        return mode;
    }
    throw new IllegalArgumentException("Unsupported value");
  }

  @NonNull
  private static SharedPreferences getSharedPreferences(@NonNull CarContext context)
  {
    return context.getSharedPreferences(ANDROID_AUTO_PREFERENCES_FILE_KEY, Context.MODE_PRIVATE);
  }

  private ThemeUtils() {}
}
