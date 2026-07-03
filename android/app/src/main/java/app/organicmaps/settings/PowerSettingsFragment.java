package app.organicmaps.settings;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.PowerManagment;
import app.organicmaps.util.ThemeSwitcher;
import app.organicmaps.util.Utils;

@Keep
public class PowerSettingsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_power;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initMapStylePrefsCallbacks();
    initUseMobileDataPrefsCallbacks();
    initPowerManagementPrefsCallbacks();
    initScreenSleepEnabledPrefsCallbacks();
    initShowOnLockScreenPrefsCallbacks();
  }

  private void initMapStylePrefsCallbacks()
  {
    final ListPreference pref = getPreference(getString(R.string.pref_map_style));
    pref.setEntryValues(new CharSequence[] {Config.UiTheme.DEFAULT, Config.UiTheme.NIGHT, Config.UiTheme.AUTO,
                                            Config.UiTheme.NAV_AUTO});
    pref.setValue(Config.UiTheme.getUiThemeSettings());
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final String themeName = (String) newValue;
      if (!Config.UiTheme.setUiThemeSettings(themeName))
        return true;
      ThemeSwitcher.INSTANCE.restart(false);
      final ThemeMode mode = ThemeMode.getInstance(themeName);
      final CharSequence summary = pref.getEntries()[mode.ordinal()];
      pref.setSummary(summary);
      return true;
    });
  }

  private void initUseMobileDataPrefsCallbacks()
  {
    final ListPreference mobilePref = getPreference(getString(R.string.pref_use_mobile_data));
    NetworkPolicy.Type curValue = Config.getUseMobileDataSettings();
    if (curValue == NetworkPolicy.Type.NOT_TODAY || curValue == NetworkPolicy.Type.TODAY)
      curValue = NetworkPolicy.Type.ASK;
    mobilePref.setValue(curValue.name());
    mobilePref.setOnPreferenceChangeListener((preference, newValue) -> {
      final String valueStr = (String) newValue;
      NetworkPolicy.Type type = NetworkPolicy.Type.valueOf(valueStr);
      Config.setUseMobileDataSettings(type);
      return true;
    });
  }

  private void initPowerManagementPrefsCallbacks()
  {
    final ListPreference powerManagementPref = getPreference(getString(R.string.pref_power_management));

    @PowerManagment.SchemeType
    int curValue = PowerManagment.getScheme();
    powerManagementPref.setValue(String.valueOf(curValue));

    powerManagementPref.setOnPreferenceChangeListener((preference, newValue) -> {
      @PowerManagment.SchemeType
      int scheme = Integer.parseInt((String) newValue);
      PowerManagment.setScheme(scheme);
      return true;
    });
  }

  private void initScreenSleepEnabledPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_keep_screen_on));
    final boolean isKeepScreenOnEnabled = Config.isKeepScreenOnEnabled();
    ((TwoStatePreference) pref).setChecked(isKeepScreenOnEnabled);
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      boolean newVal = (Boolean) newValue;
      if (isKeepScreenOnEnabled != newVal)
      {
        Config.setKeepScreenOnEnabled(newVal);
      }
      return true;
    });
  }

  private void initShowOnLockScreenPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_show_on_lock_screen));
    final boolean isShowOnLockScreenEnabled = Config.isShowOnLockScreenEnabled();
    ((TwoStatePreference) pref).setChecked(isShowOnLockScreenEnabled);
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      boolean newVal = (Boolean) newValue;
      if (isShowOnLockScreenEnabled != newVal)
      {
        Config.setShowOnLockScreenEnabled(newVal);
        Utils.showOnLockScreen(newVal, requireActivity());
      }
      return true;
    });
  }

  enum ThemeMode
  {
    DEFAULT(Config.UiTheme.DEFAULT),
    NIGHT(Config.UiTheme.NIGHT),
    AUTO(Config.UiTheme.AUTO),
    NAV_AUTO(Config.UiTheme.NAV_AUTO);

    @NonNull
    private final String mMode;

    ThemeMode(@NonNull String mode)
    {
      mMode = mode;
    }

    @NonNull
    public static ThemeMode getInstance(@NonNull String src)
    {
      for (ThemeMode each : values())
      {
        if (each.mMode.equals(src))
          return each;
      }
      return AUTO;
    }
  }
}
