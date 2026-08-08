package app.organicmaps.settings;
import androidx.annotation.Keep;

import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.editor.MapLanguagesFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.editor.data.Language;
import app.organicmaps.sdk.settings.MapLanguageCode;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.PowerManagment;
import java.util.Locale;

@Keep
public class MapAppearanceSettingsFragment extends BaseXmlSettingsFragment implements MapLanguagesFragment.Listener
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_map_appearance;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    init3dModePrefsCallbacks();
    initLargeFontSizePrefsCallbacks();
    initTransliterationPrefsCallbacks();
    initAlternativeMapLanguageHandlingCallbacks();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateMapLanguageCodeSummary();
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
    return super.onPreferenceTreeClick(preference);
  }

  @Override
  public void onMapLanguageSelected(Language language)
  {
    MapLanguageCode.setMapLanguageCode(language.code);
    getSettingsActivity().onBackPressed();
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

  private void init3dModePrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d_buildings));
    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);

    // Check power management: high-power mode disables 3D buildings
    @PowerManagment.SchemeType int powerScheme = PowerManagment.getScheme();
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
}
