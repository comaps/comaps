package app.organicmaps.settings;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.util.Utils;

@Keep
public class NavigationSettingsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_navigation;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initMeasureUnitsPrefsCallbacks();
    initPerspectivePrefsCallbacks();
    initAutoZoomPrefsCallbacks();
    initAndroidAutoSupportPrefsCallback();
    initAutoRerouteCallback();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateVoiceInstructionsPrefsSummary();
    updateRoutingSettingsPrefsSummary();
  }

  @Override
  public boolean onPreferenceTreeClick(Preference preference)
  {
    final String key = preference.getKey();
    if (key == null)
      return super.onPreferenceTreeClick(preference);

    if (key.equals(getString(R.string.pref_tts_screen)))
    {
      getSettingsActivity().stackFragment(VoiceInstructionsSettingsFragment.class,
                                          getString(R.string.pref_tts_enable_title), null);
    }
    else if (key.equals(getString(R.string.pref_android_auto_support)))
    {
      if (!Utils.isAndroidAutoSupported(requireContext()))
        Utils.openUrl(requireContext(), "https://www.comaps.app/support/how-to-use-android-auto/");
    }
    return super.onPreferenceTreeClick(preference);
  }

  private void updateVoiceInstructionsPrefsSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_tts_screen));
    pref.setSummary(Config.TTS.isEnabled() ? R.string.on : R.string.off);
  }

  private void initAutoRerouteCallback()
  {
    TwoStatePreference autoReroutePref =
        getPreference(getString(app.organicmaps.sdk.R.string.pref_enable_auto_reroute));
    Framework.nativeSetAutoReroute(autoReroutePref.isChecked());

    autoReroutePref.setOnPreferenceChangeListener((preference, value) -> {
      Framework.nativeSetAutoReroute((boolean) value);
      return true;
    });
  }

  private void updateRoutingSettingsPrefsSummary()
  {
    final Preference pref = getPreference(getString(R.string.prefs_routing));
    pref.setSummary(RoutingOptions.hasAnyOptions(RoutingController.get().getLastRouterType()) ? R.string.on
                                                                                              : R.string.off);
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

  private void initPerspectivePrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_3d));
    final Framework.Params3dMode _3d = new Framework.Params3dMode();
    Framework.nativeGet3dMode(_3d);
    pref.setChecked(_3d.enabled);
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      Framework.Params3dMode current = new Framework.Params3dMode();
      Framework.nativeGet3dMode(current);
      Framework.nativeSet3dMode((Boolean) newValue, current.buildings);
      return true;
    });
  }

  private void initAutoZoomPrefsCallbacks()
  {
    final TwoStatePreference pref = getPreference(getString(R.string.pref_auto_zoom));
    boolean autozoomEnabled = Framework.nativeGetAutoZoomEnabled();
    pref.setChecked(autozoomEnabled);
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      Framework.nativeSetAutoZoomEnabled((boolean) newValue);
      return true;
    });
  }

  private void initAndroidAutoSupportPrefsCallback()
  {
    Preference aASupportPref = getPreference(getString(R.string.pref_android_auto_support));
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
    {
      boolean androidAutoState = Utils.isAndroidAutoSupported(requireContext());
      aASupportPref.setSummary(androidAutoState ? R.string.pref_aa_support_summary_yes : R.string.pref_aa_support_summary_no);
      aASupportPref.setIcon(androidAutoState ? R.drawable.ic_car_enabled : R.drawable.ic_car_disabled);
    }
    else
    {
      aASupportPref.setVisible(false);
    }
  }
}
