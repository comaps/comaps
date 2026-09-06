package app.organicmaps.util;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.location.Location;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDelegate;
import app.organicmaps.MwmApplication;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.MapAppearance;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.Calendar;

public enum ThemeSwitcher
{
  INSTANCE;

  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;

  private final Runnable mAutoThemeChecker = new Runnable() {
    @Override
    public void run()
    {
      boolean navAuto = RoutingController.get().isNavigating() && ThemeUtils.isNavAutoTheme();
      // Cancel old checker
      UiThread.cancelDelayedTasks(mAutoThemeChecker);

      String theme;
      if (navAuto || ThemeUtils.isAutoTheme())
      {
        UiThread.runLater(mAutoThemeChecker, CHECK_INTERVAL_MS);
        theme = calcAutoTheme();
      }
      else
      {
        // Happens when exiting the Navigation mode. Should restore the light.
        theme = Config.UiTheme.DEFAULT;
      }

      setTheme(theme);
    }
  };

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  public void initialize(@NonNull Context context)
  {
    mContext = context;
  }

  /**
   * Changes the UI theme of application and the map style if necessary.
   *
   * @param isRendererActive Indicates whether OpenGL renderer is active or not. Must be
   *                         <code>true</code> only if the map is rendered and visible on the screen
   *                         at this moment, otherwise <code>false</code>.
   */
  @androidx.annotation.UiThread
  public void restart(boolean isRendererActive)
  {
    String theme = Config.UiTheme.getUiThemeSettings();
    if (ThemeUtils.isAutoTheme() || ThemeUtils.isNavAutoTheme())
    {
      mAutoThemeChecker.run();
      return;
    }

    UiThread.cancelDelayedTasks(mAutoThemeChecker);
    setTheme(theme);
  }

  private void setTheme(@NonNull String theme)
  {
    UiModeManager uiModeManager = (UiModeManager) mContext.getSystemService(Context.UI_MODE_SERVICE);
    String oldTheme = Config.UiTheme.getCurrent();

    MapAppearance mapAppearance;
    if (Config.UiTheme.isNight(theme))
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_YES);
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

      mapAppearance = MapAppearance.Dark;
    }
    else
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_NO);
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);

      mapAppearance = MapAppearance.Light;
    }

    Framework.nativeSwitchToUsingVehicleStyle(RoutingController.get().isVehicleNavigation());

    if (!theme.equals(oldTheme))
    {
      Config.UiTheme.setCurrent(theme);
      DownloaderStatusIcon.clearCache();

      final Activity a = MwmApplication.from(mContext).getTopActivity();
      if (a != null && !a.isFinishing())
        a.recreate();
    }
    else
    {
      // If the UI theme is not changed we just need to change the map style if needed.
      final MapAppearance currentMapAppearance = MapAppearance.get();
      if (currentMapAppearance == mapAppearance)
        return;
      SetMapAppearance(mapAppearance);
    }
  }

  private void SetMapAppearance(MapAppearance mapAppearance)
  {
    // Because of the distinct behavior in auto theme, Android Auto employs its own mechanism for theme switching.
    // For the Android Auto theme switcher, please consult the app.organicmaps.car.util.ThemeUtils module.
    if (MwmApplication.from(mContext).getDisplayManager().isCarDisplayUsed())
      return;

    MapAppearance.set(mapAppearance);
  }

  /**
   * Determine light/dark theme based on time and location,
   * or fall back to time-based (06:00-18:00) when there's no location fix
   *
   * @return theme_light/dark string
   */
  @NonNull
  private String calcAutoTheme()
  {
    final Location last = MwmApplication.from(mContext).getLocationHelper().getSavedLocation();
    boolean day;

    if (last != null)
    {
      long currentTime = System.currentTimeMillis() / 1000;
      day = Framework.nativeIsDayTime(currentTime, last.getLatitude(), last.getLongitude());
    }
    else
    {
      int currentHour = Calendar.getInstance().get(Calendar.HOUR_OF_DAY);
      day = (currentHour < 18 && currentHour > 6);
    }

    return (day ? Config.UiTheme.DEFAULT : Config.UiTheme.NIGHT);
  }
}
