package app.organicmaps.settings;
import static app.organicmaps.leftbutton.LeftButtonsHolder.DISABLE_BUTTON_CODE;

import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.os.LocaleListCompat;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceManager;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.dialog.CustomMapServerDialog;
import app.organicmaps.downloader.OnmapDownloader;
import app.organicmaps.editor.MapLanguagesFragment;
import app.organicmaps.leftbutton.LeftButton;
import app.organicmaps.leftbutton.LeftButtonsHolder;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.editor.data.Language;
import app.organicmaps.sdk.settings.MapLanguageCode;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.PowerManagment;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.sdk.util.log.LogsManager;
import app.organicmaps.util.Utils;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Locale;

@Keep
public class GeneralSettingsFragment
    extends BaseXmlSettingsFragment implements MapLanguagesFragment.Listener, AppLanguagesFragment.Listener
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_general;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initMeasureUnitsPrefsCallbacks();
    initZoomPrefsCallbacks();
    initLeftButtonPrefs();
    init3dModePrefsCallbacks();
    initAutoDownloadPrefsCallbacks();
    initLargeFontSizePrefsCallbacks();
    initTransliterationPrefsCallbacks();
    initAlternativeMapLanguageHandlingCallbacks();
    initStoragePrefCallbacks();
    initLoggingEnabledPrefsCallbacks();
    initEmulationBadStorage();
    initOpenExternalLinksPrefsCallback();
    initCustomMapDownloadUrlPrefsCallbacks();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateMapLanguageCodeSummary();
    updateAppLanguageCodeSummary();
  }

  @Override
  public boolean onPreferenceTreeClick(Preference preference)
  {
    final String key = preference.getKey();
    if (key == null)
      return super.onPreferenceTreeClick(preference);

    if (key.equals(getString(R.string.pref_map_locale)))
    {
      MapLanguagesFragment langFragment = (MapLanguagesFragment) getSettingsActivity().stackFragment(
          MapLanguagesFragment.class, getString(R.string.change_map_locale), null);
      langFragment.setListener(this);
    }
    else if (key.equals(getString(R.string.pref_app_locale)))
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
    else if (key.equals(getString(R.string.pref_backup)))
    {
      getSettingsActivity().stackFragment(BackupSettingsFragment.class, getString(R.string.pref_backup_title), null);
    }
    else if (key.equals(getString(R.string.pref_open_external_links)))
    {
      final Intent intent = new Intent(Settings.ACTION_APP_OPEN_BY_DEFAULT_SETTINGS);
      intent.setData(Uri.fromParts("package", requireContext().getPackageName(), null));
      startActivity(intent);
    }
    return super.onPreferenceTreeClick(preference);
  }

  @Override
  public void onMapLanguageSelected(Language language)
  {
    MapLanguageCode.setMapLanguageCode(language.code);
    getSettingsActivity().onBackPressed();
  }

  @Override
  public void onAppLanguageSelected()
  {
    getSettingsActivity().onBackPressed();
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

  private void updateMapLanguageCodeSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_map_locale));
    String mapLanguageCode = MapLanguageCode.getMapLanguageCode();
    if (mapLanguageCode.equals(Language.AUTO_LANG_CODE))
      pref.setSummary(R.string.auto);
    else if (mapLanguageCode.equals(Language.DEFAULT_LANG_CODE))
      pref.setSummary(R.string.pref_maplanguage_local);
    else
      pref.setSummary(new Locale(mapLanguageCode).getDisplayLanguage());
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

  private void initLargeFontSizePrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_large_fonts_size));
    ((TwoStatePreference) pref).setChecked(Config.isLargeFontsSize());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean oldVal = Config.isLargeFontsSize();
      final boolean newVal = (Boolean) newValue;
      if (oldVal != newVal)
        Config.setLargeFontsSize(newVal);
      return true;
    });
  }

  private void initAlternativeMapLanguageHandlingCallbacks()
  {
    final ListPreference pref = getPreference(getString(R.string.pref_alt_map_lang_handling_key));
    pref.setValue(String.valueOf(Config.getAlternativeMapLanguageHandling()));
    pref.setSummary(pref.getEntry());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final int alternativeMapLanguageHandling = Integer.parseInt((String) newValue);
      Config.setAlternativeMapLanguageHandling(alternativeMapLanguageHandling);
      preference.setSummary(pref.getEntries()[alternativeMapLanguageHandling]);
      return true;
    });
  }

  private void initTransliterationPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_transliteration));
    ((TwoStatePreference) pref).setChecked(Config.isTransliteration());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean oldVal = Config.isTransliteration();
      final boolean newVal = (Boolean) newValue;
      if (oldVal != newVal)
        Config.setTransliteration(newVal);
      return true;
    });
  }

  private void initLoggingEnabledPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_enable_logging));
    ((TwoStatePreference) pref).setChecked(LogsManager.INSTANCE.isFileLoggingEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      if (!LogsManager.INSTANCE.setFileLoggingEnabled((Boolean) newValue))
      {
        Utils.showSnackbar(requireView(), "ERROR: Can't create a logs folder!");
        return false;
      }
      return true;
    });
  }

  private void initEmulationBadStorage()
  {
    final Preference pref = findPreference(getString(R.string.pref_emulate_bad_external_storage));
    if (pref == null)
      return;
    if (!SharedPropertiesUtils.shouldShowEmulateBadStorageSetting())
      pref.setVisible(false);
  }

  private void init3dModePrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d_buildings));
    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);

    // Check power management: high-power mode disables 3D buildings
    @PowerManagment.SchemeType
    int powerScheme = PowerManagment.getScheme();
    if (powerScheme == PowerManagment.HIGH)
    {
      pref.setShouldDisableView(true);
      pref.setEnabled(false);
      pref.setSummary(getString(R.string.pref_map_3d_buildings_disabled_summary));
      pref.setChecked(false);
    }
    else
    {
      pref.setShouldDisableView(false);
      pref.setEnabled(true);
      pref.setSummary("");
      pref.setChecked(_3d.buildings);
    }

    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      Framework.Params3dMode current = new Framework.Params3dMode();
      Framework.nativeGet3dMode(current);
      Framework.nativeSet3dMode(current.enabled, (Boolean) newValue);
      return true;
    });
  }

  private void initAutoDownloadPrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_autodownload));
    pref.setChecked(Config.isAutodownloadEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean value = (boolean) newValue;
      Config.setAutodownloadEnabled(value);
      if (value)
        OnmapDownloader.setAutodownloadLocked(false);
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

  private void initMeasureUnitsPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_munits));
    ((ListPreference) pref).setValue(String.valueOf(UnitLocale.getUnits()));
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      UnitLocale.setUnits(Integer.parseInt((String) newValue));
      return true;
    });
  }

  private void initStoragePrefCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_storage));
    pref.setOnPreferenceClickListener(preference -> {
      if (app.organicmaps.sdk.downloader.MapManager.nativeIsDownloading())
      {
        new com.google.android.material.dialog.MaterialAlertDialogBuilder(requireActivity())
            .setTitle(R.string.downloading_is_active)
            .setMessage(R.string.cant_change_this_setting)
            .setPositiveButton(R.string.ok, null)
            .show();
      }
      else
      {
        getSettingsActivity().stackFragment(StoragePathFragment.class, getString(R.string.maps_storage), null);
      }
      return true;
    });
  }

  private void initOpenExternalLinksPrefsCallback()
  {
    Preference openExternalLinksPref = getPreference(getString(R.string.pref_open_external_links));
    openExternalLinksPref.setVisible(Build.VERSION.SDK_INT >= Build.VERSION_CODES.S);
  }

  private void initCustomMapDownloadUrlPrefsCallbacks()
  {
    Preference customUrlPref = getPreference(getString(R.string.pref_custom_map_download_url));

    SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(requireContext());

    String current = prefs.getString(getString(R.string.pref_custom_map_download_url), "");
    String normalizedUrl = Framework.normalizeServerUrl(current);

    customUrlPref.setSummary(normalizedUrl.isEmpty() ? getString(R.string.download_resources_custom_url_summary_none)
                                                     : normalizedUrl);

    Framework.applyCustomMapDownloadUrl(requireContext(), normalizedUrl);

    customUrlPref.setOnPreferenceClickListener(preference -> {
      CustomMapServerDialog.show(
          requireContext(),
          url
          -> preference.setSummary(url.isEmpty() ? getString(R.string.download_resources_custom_url_summary_none)
                                                 : url));
      return true;
    });
  }
}
