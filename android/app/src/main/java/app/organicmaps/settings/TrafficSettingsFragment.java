package app.organicmaps.settings;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.EditTextPreference;
import androidx.preference.MultiSelectListPreference;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.sdk.traffxml.AndroidTransport;
import app.organicmaps.sdk.util.Config;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

@Keep
public class TrafficSettingsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_traffic;
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateTrafficHttpUrlSummary();
    updateTrafficAppsSummary();
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initTrafficHttpEnabledPrefsCallbacks();
    initTrafficHttpUrlPrefsCallbacks();
    initTrafficAppsPrefs();
    initTrafficLegacyEnabledPrefsCallbacks();
  }

  private void initTrafficHttpEnabledPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_traffic_http_enabled));

    ((TwoStatePreference)pref).setChecked(Config.getTrafficHttpEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean oldVal = Config.getTrafficHttpEnabled();
      final boolean newVal = (Boolean) newValue;
      if (oldVal != newVal)
        Config.setTrafficHttpEnabled(newVal);

      return true;
    });
  }

  private void initTrafficHttpUrlPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_traffic_http_url));

    ((EditTextPreference)pref).setText(Config.getTrafficHttpUrl());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final String oldVal = Config.getTrafficHttpUrl();
      final String newVal = (String) newValue;
      if (!oldVal.equals(newVal))
        Config.setTrafficHttpUrl(newVal);

      return true;
    });
  }

  private void initTrafficAppsPrefs()
  {
    final MultiSelectListPreference pref = getPreference(getString(R.string.pref_traffic_apps));
    
    PackageManager pm = getContext().getPackageManager();
    List<ResolveInfo> receivers = pm.queryBroadcastReceivers(new Intent(AndroidTransport.ACTION_TRAFF_GET_CAPABILITIES), 0);
    
    if (receivers == null || receivers.isEmpty())
    {
      pref.setSummary(R.string.traffic_apps_not_available);
      pref.setEnabled(false);
      return;
    }
    
    pref.setEnabled(true);

    List<String> entryList = new ArrayList<>(receivers.size());
    List<String> valueList = new ArrayList<>(receivers.size());

    for (ResolveInfo receiver : receivers)
    {
      // friendly name
      entryList.add(receiver.loadLabel(pm).toString());
      // actual value (we just need the package name, broadcasts are sent to any receiver in the package)
      valueList.add(receiver.activityInfo.applicationInfo.packageName);
    }
    
    pref.setEntries(entryList.toArray(new CharSequence[0]));
    pref.setEntryValues(valueList.toArray(new CharSequence[0]));
    
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      // newValue is a Set<String>, each item is a package ID
      String[] apps = ((Set<String>)newValue).toArray(new String[0]);
      Config.setTrafficApps(apps);
      updateTrafficAppsSummary();

      return true;
    });
  }

  private void initTrafficLegacyEnabledPrefsCallbacks()
  {
    final Preference pref = getPreference(getString(R.string.pref_traffic_legacy_enabled));

    ((TwoStatePreference)pref).setChecked(Config.getTrafficLegacyEnabled());
    pref.setOnPreferenceChangeListener((preference, newValue) -> {
      final boolean oldVal = Config.getTrafficLegacyEnabled();
      final boolean newVal = (Boolean) newValue;
      if (oldVal != newVal)
        Config.setTrafficLegacyEnabled(newVal);

      return true;
    });
  }

  private void updateTrafficHttpUrlSummary()
  {
    final Preference pref = getPreference(getString(R.string.pref_traffic_http_url));
    String summary = Config.getTrafficHttpUrl();
    if (summary.length() == 0)
      pref.setSummary(R.string.traffic_http_url_not_set);
    else
      pref.setSummary(summary);
  }

  private void updateTrafficAppsSummary()
  {
    final MultiSelectListPreference pref = getPreference(getString(R.string.pref_traffic_apps));
    /*
     * If the preference is disabled, it has not been initialized. This is the case if no TraFF
     * apps were found. The code below would crash when trying to access the entries, and there
     * is no need to update the summary if the setting cannot be changed.
     */
    if (!pref.isEnabled())
      return;
    String[] apps = Config.getTrafficApps();
    if (apps.length == 0)
      pref.setSummary(R.string.traffic_apps_none_selected);
    else
    {
      String summary = "";
      for (int i = 0; i < apps.length; i++)
      {
        if (i > 0)
          summary = summary + ", ";
        int index = pref.findIndexOfValue(apps[i]);
        if (i >= 0)
          summary = summary + pref.getEntries()[index];
        else
          summary = summary + apps[i];
      }
      pref.setSummary(summary);
    }
  }

}
