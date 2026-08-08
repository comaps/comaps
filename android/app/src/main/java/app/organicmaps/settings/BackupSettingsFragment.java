package app.organicmaps.settings;
import androidx.annotation.Keep;

import static app.organicmaps.backup.BackupUtils.formatReadableFolderPath;
import static app.organicmaps.backup.BackupUtils.getMaxBackups;
import static app.organicmaps.sdk.util.StorageUtils.isFolderWritable;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceManager;
import app.organicmaps.R;
import app.organicmaps.backup.LocalBackupManager;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.text.DateFormat;

@Keep
public class BackupSettingsFragment extends BaseXmlSettingsFragment
{
  private ActivityResultLauncher<Intent> folderPickerLauncher;

  private static final String TAG = LocalBackupManager.class.getSimpleName();
  public static final String BACKUP_FOLDER_PATH_KEY = "backup_location";
  public static final String LAST_BACKUP_TIME_KEY = "last_backup_time";
  private static final String BACKUP_NOW_KEY = "backup_now";
  public static final String BACKUP_INTERVAL_KEY = "backup_history_interval";
  public static final String MAX_BACKUPS_KEY = "backup_history_count";
  public static final int MAX_BACKUPS_DEFAULT_COUNT = 10;
  public static final String DEFAULT_BACKUP_INTERVAL = "86400000"; // 24 hours in ms

  private LocalBackupManager mBackupManager;
  private SharedPreferences prefs;

  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_backup;
  }

  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  Preference backupLocationOption;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  ListPreference backupIntervalOption;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  Preference maxBackupsOption;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  Preference backupNowOption;
  @NonNull
  @SuppressWarnings("NotNullFieldNotInitialized")
  Preference advancedCategory;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    folderPickerLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
      boolean isSuccess = false;

      String lastFolderPath = prefs.getString(BACKUP_FOLDER_PATH_KEY, null);

      if (result.getResultCode() == Activity.RESULT_OK)
      {
        Intent data = result.getData();
        Logger.i(TAG, "Folder selection result: " + data);
        if (data == null)
          return;

        Uri uri = data.getData();
        if (uri != null)
        {
          takePersistableUriPermission(uri);
          Logger.i(TAG, "Backup location changed to " + uri);
          prefs.edit().putString(BACKUP_FOLDER_PATH_KEY, uri.toString()).apply();
          setFormattedBackupPath(uri);

          runBackup();

          isSuccess = true;
        }
        else
        {
          Logger.w(TAG, "Folder selection result is null");
        }
      }
      else if (result.getResultCode() == Activity.RESULT_CANCELED)
      {
        Logger.w(TAG, "User canceled folder selection");
        if (TextUtils.isEmpty(lastFolderPath))
        {
          prefs.edit().putString(BACKUP_FOLDER_PATH_KEY, null).apply();
          Logger.i(TAG, "Backup settings reset");
          initBackupLocationOption();
        }
        else if (isFolderWritable(requireActivity(), lastFolderPath))
        {
          Logger.i(TAG, "Backup location not changed, using previous value " + lastFolderPath);
          isSuccess = true;
        }
        else
        {
          Logger.e(TAG, "Backup location not changed, but last folder is not writable: " + lastFolderPath);
        }
      }

      resetLastBackupTime();
      updateStatusSummaryOption();

      Logger.i(TAG, "Folder selection result: " + isSuccess);
      applyAdvancedSettings(isSuccess);
    });
  }

  @Override
  public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
  {
    super.onCreatePreferences(savedInstanceState, rootKey);

    prefs = PreferenceManager.getDefaultSharedPreferences(requireContext());
    backupLocationOption = findPreference(BACKUP_FOLDER_PATH_KEY);
    backupIntervalOption = findPreference(BACKUP_INTERVAL_KEY);
    maxBackupsOption = findPreference(MAX_BACKUPS_KEY);
    backupNowOption = findPreference(BACKUP_NOW_KEY);

    initBackupLocationOption();
    initBackupIntervalOption();
    initMaxBackupsOption();
    initBackupNowOption();
  }

  private void initBackupLocationOption()
  {
    String storedFolderPath = prefs.getString(BACKUP_FOLDER_PATH_KEY, null);
    boolean isEnabled = false;
    if (!TextUtils.isEmpty(storedFolderPath))
    {
      if (isFolderWritable(requireContext(), storedFolderPath))
      {
        setFormattedBackupPath(Uri.parse(storedFolderPath));
        isEnabled = true;
      }
      else
      {
        Logger.e(TAG, "Backup location is not available, path: " + storedFolderPath);
        showBackupErrorAlertDialog(requireContext().getString(R.string.dialog_report_error_missing_folder));
        backupLocationOption.setSummary(
            requireContext().getString(R.string.pref_backup_now_summary_folder_unavailable));
      }
    }
    else
    {
      backupLocationOption.setSummary(requireContext().getString(R.string.pref_backup_location_summary_initial));
    }

    applyAdvancedSettings(isEnabled);

    backupLocationOption.setOnPreferenceClickListener(preference -> {
      launchFolderPicker();

      return true;
    });
  }

  private void setFormattedBackupPath(@NonNull Uri uri)
  {
    backupLocationOption.setSummary(formatReadableFolderPath(requireContext(), uri));
  }

  private void initBackupIntervalOption()
  {
    String backupInterval = prefs.getString(BACKUP_INTERVAL_KEY, DEFAULT_BACKUP_INTERVAL);

    CharSequence entry = getEntryForValue(backupIntervalOption, backupInterval);
    if (entry != null)
      backupIntervalOption.setSummary(entry);

    backupIntervalOption.setOnPreferenceChangeListener((preference, newValue) -> {
      CharSequence newEntry = getEntryForValue(backupIntervalOption, newValue.toString());
      Logger.i(TAG, "auto backup interval changed to " + newEntry);
      if (newEntry != null)
        backupIntervalOption.setSummary(newEntry);

      return true;
    });
  }

  private void initMaxBackupsOption()
  {
    maxBackupsOption.setSummary(String.valueOf(getMaxBackups(prefs)));

    maxBackupsOption.setOnPreferenceChangeListener((preference, newValue) -> {
      maxBackupsOption.setSummary(newValue.toString());

      return true;
    });
  }

  private void initBackupNowOption()
  {
    updateStatusSummaryOption();
    backupNowOption.setOnPreferenceClickListener(preference -> {
      runBackup();

      return true;
    });
  }

  private void updateStatusSummaryOption()
  {
    long lastBackupTime = prefs.getLong(LAST_BACKUP_TIME_KEY, 0L);

    String summary;
    if (lastBackupTime > 0)
    {
      String time = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(lastBackupTime);
      summary = requireContext().getString(R.string.pref_backup_status_summary_success) + ": " + time;
    }
    else
    {
      summary = requireContext().getString(R.string.pref_backup_now_summary);
    }

    backupNowOption.setSummary(summary);
  }

  private void resetLastBackupTime()
  {
    prefs.edit().remove(LAST_BACKUP_TIME_KEY).apply();
  }

  private void applyAdvancedSettings(boolean isBackupEnabled)
  {
    backupIntervalOption.setVisible(isBackupEnabled);
    maxBackupsOption.setVisible(isBackupEnabled);
    backupNowOption.setVisible(isBackupEnabled);
  }

  private void runBackup()
  {
    String currentFolderPath = prefs.getString(BACKUP_FOLDER_PATH_KEY, null);
    if (!TextUtils.isEmpty(currentFolderPath))
    {
      if (isFolderWritable(requireContext(), currentFolderPath))
      {
        mBackupManager = new LocalBackupManager(requireActivity(), currentFolderPath, getMaxBackups(prefs));
        mBackupManager.setListener(new LocalBackupManager.Listener() {
          @Override
          public void onBackupStarted()
          {
            Logger.i(TAG, "Manual backup started");

            backupNowOption.setEnabled(false);
            backupNowOption.setSummary(R.string.pref_backup_now_summary_progress);
          }

          @Override
          public void onBackupFinished()
          {
            Logger.i(TAG, "Manual backup successful");

            backupNowOption.setEnabled(true);
            backupNowOption.setSummary(R.string.pref_backup_now_summary_ok);

            prefs.edit().putLong(LAST_BACKUP_TIME_KEY, System.currentTimeMillis()).apply();
          }

          @Override
          public void onBackupFailed(LocalBackupManager.ErrorCode errorCode)
          {
            String errorMessage;
            if (errorCode == LocalBackupManager.ErrorCode.EMPTY_CATEGORY)
            {
              errorMessage = requireContext().getString(R.string.pref_backup_now_summary_empty_lists);
              Logger.i(TAG, "Nothing to backup");
            }
            else
            {
              errorMessage = requireContext().getString(R.string.pref_backup_now_summary_failed);
              Logger.e(TAG, "Manual backup has failed: " + errorCode);
            }

            backupNowOption.setEnabled(true);
            backupNowOption.setSummary(errorMessage);

            if (errorCode != LocalBackupManager.ErrorCode.EMPTY_CATEGORY)
            {
              showBackupErrorAlertDialog(requireContext().getString(R.string.dialog_report_error_with_logs));
            }
          }
        });

        mBackupManager.doBackup();
      }
      else
      {
        backupNowOption.setSummary(R.string.pref_backup_now_summary_folder_unavailable);
        showBackupErrorAlertDialog(requireContext().getString(R.string.dialog_report_error_missing_folder));
        Logger.e(TAG, "Manual backup error: folder " + currentFolderPath + " unavailable");
      }
    }
    else
    {
      backupNowOption.setSummary(R.string.pref_backup_now_summary_folder_unavailable);
      Logger.e(TAG, "Manual backup error: no folder selected");
    }
  }

  private void launchFolderPicker()
  {
    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
    intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
    intent.putExtra("android.content.extra.SHOW_ADVANCED", true);

    PackageManager packageManager = requireActivity().getPackageManager();
    if (intent.resolveActivity(packageManager) != null)
      folderPickerLauncher.launch(intent);
    else
      showNoFileManagerError();
  }

  private void showNoFileManagerError()
  {
    new MaterialAlertDialogBuilder(requireActivity())
        .setMessage(R.string.error_no_file_manager_app)
        .setPositiveButton(android.R.string.ok, (dialog, which) -> dialog.dismiss())
        .show();
  }

  private void showBackupErrorAlertDialog(String message)
  {
    requireActivity().runOnUiThread(
        ()
            -> new MaterialAlertDialogBuilder(requireActivity())
                   .setTitle(R.string.pref_backup_now_summary_failed)
                   .setMessage(message)
                   .setPositiveButton(android.R.string.ok, (dialog, which) -> dialog.dismiss())
                   .show());
  }

  private void takePersistableUriPermission(Uri uri)
  {
    requireContext().getContentResolver().takePersistableUriPermission(
        uri, Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
  }

  @Nullable
  public static CharSequence getEntryForValue(@NonNull ListPreference listPref, @NonNull CharSequence value)
  {
    CharSequence[] entryValues = listPref.getEntryValues();
    CharSequence[] entries = listPref.getEntries();

    if (entryValues == null || entries == null)
      return null;

    for (int i = 0; i < entryValues.length; i++)
    {
      if (entryValues[i].equals(value))
        return entries[i];
    }
    return null;
  }
}
