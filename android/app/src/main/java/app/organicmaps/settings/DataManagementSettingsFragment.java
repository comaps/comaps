package app.organicmaps.settings;
import androidx.annotation.Keep;

import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.downloader.OnmapDownloader;
import app.organicmaps.sdk.util.Config;

@Keep
public class DataManagementSettingsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_data_management;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initStoragePrefCallbacks();
    initBackupPrefCallback();
    initAutoDownloadPrefsCallbacks();
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

  private void initBackupPrefCallback()
  {
    final Preference pref = getPreference(getString(R.string.pref_backup));
    pref.setOnPreferenceClickListener(preference -> {
      getSettingsActivity().stackFragment(BackupSettingsFragment.class, getString(R.string.pref_backup_title), null);
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
}
