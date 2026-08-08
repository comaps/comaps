package app.organicmaps.settings;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;

import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.editor.ProfileActivity;
import app.organicmaps.help.HelpActivity;
import app.organicmaps.sdk.editor.OsmOAuth;

public class SettingsPrefsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_main;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateProfileSettingsPrefsSummary();
    updateAboutSummary();
  }

  @Override
  public boolean onPreferenceTreeClick(Preference preference)
  {
    final String key = preference.getKey();
    if (key != null && key.equals(getString(R.string.pref_osm_profile)))
    {
      startActivity(new Intent(requireActivity(), ProfileActivity.class));
    }
    else if (key != null && key.equals(getString(R.string.pref_about)))
    {
      startActivity(new Intent(requireActivity(), HelpActivity.class));
    }
    return super.onPreferenceTreeClick(preference);
  }

  private void updateProfileSettingsPrefsSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_osm_profile));
    if (OsmOAuth.isAuthorized())
      pref.setSummary(OsmOAuth.getUsername());
    else
      pref.setSummary(R.string.not_signed_in);
  }

  private void updateAboutSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_about));
    pref.setSummary(getString(R.string.pref_about_summary, BuildConfig.VERSION_NAME));
  }
}
