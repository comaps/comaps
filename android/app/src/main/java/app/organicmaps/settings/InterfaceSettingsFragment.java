package app.organicmaps.settings;
import androidx.annotation.Keep;

import static app.organicmaps.leftbutton.LeftButtonsHolder.DISABLE_BUTTON_CODE;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.os.LocaleListCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.leftbutton.LeftButton;
import app.organicmaps.leftbutton.LeftButtonsHolder;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.util.ThemeSwitcher;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Locale;

@Keep
public class InterfaceSettingsFragment extends BaseXmlSettingsFragment implements AppLanguagesFragment.Listener
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_interface;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initMapStylePrefsCallbacks();
    initZoomPrefsCallbacks();
    initLeftButtonPrefs();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateAppLanguageCodeSummary();
  }

  @Override
  public boolean onPreferenceTreeClick(Preference preference)
  {
    final String key = preference.getKey();
    if (key == null)
      return super.onPreferenceTreeClick(preference);

    if (key.equals(getString(R.string.pref_app_locale)))
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
      {
        Intent intent = new Intent(Settings.ACTION_APP_LOCALE_SETTINGS);
        intent.setData(Uri.fromParts("package", requireContext().getPackageName(), null));
        startActivity(intent);
      }
      else
      {
        AppLanguagesFragment langFragment = (AppLanguagesFragment) getSettingsActivity().stackFragment(
            AppLanguagesFragment.class, getString(R.string.change_app_locale), null);
        langFragment.setListener(this);
      }
    }
    return super.onPreferenceTreeClick(preference);
  }

  @Override
  public void onAppLanguageSelected()
  {
    getSettingsActivity().onBackPressed();
  }

  private void updateAppLanguageCodeSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_app_locale));
    pref.setVisible(true);
    LocaleListCompat appLocales = AppCompatDelegate.getApplicationLocales();
    Locale currentLocale = appLocales.get(0);
    if (appLocales.isEmpty())
      pref.setSummary(getString(R.string.setting_value_system_default));
    else if (currentLocale != null)
      pref.setSummary(currentLocale.getDisplayLanguage());
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

  private void initZoomPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_show_zoom_buttons));
    ((TwoStatePreference) pref).setChecked(Config.showZoomButtons());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      Config.setShowZoomButtons((boolean) newValue);
      return true;
    });
  }

  private void initLeftButtonPrefs()
  {
    final String leftButtonPreferenceKey = getString(R.string.pref_left_button);
    final ListPreference pref = getPreference(leftButtonPreferenceKey);
    LeftButtonsHolder holder = LeftButtonsHolder.getInstance(requireContext());

    LeftButton currentButton = holder.getActiveButton();
    Collection<LeftButton> buttons = holder.getAllButtons();

    List<String> entryList = new ArrayList<>(buttons.size());
    List<String> valueList = new ArrayList<>(buttons.size());

    for (LeftButton button : buttons)
    {
      entryList.add(button.getPrefsName());
      valueList.add(button.getCode());
    }

    pref.setEntries(entryList.toArray(new CharSequence[0]));
    pref.setEntryValues(valueList.toArray(new CharSequence[0]));

    if (currentButton != null)
    {
      pref.setSummary(currentButton.getPrefsName());
      pref.setValue(currentButton.getCode());
    }
    else
    {
      pref.setSummary(R.string.pref_left_button_disable);
      pref.setValue(DISABLE_BUTTON_CODE);
    }

    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      int index = pref.findIndexOfValue(newValue.toString());
      if (index >= 0)
        pref.setSummary(pref.getEntries()[index]);

      Intent intent = new Intent();
      intent.putExtra(leftButtonPreferenceKey, newValue.toString());
      requireActivity().setResult(-1, intent);
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
