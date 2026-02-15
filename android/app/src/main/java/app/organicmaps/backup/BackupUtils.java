package app.organicmaps.backup;

import static app.organicmaps.sdk.util.StorageUtils.isFolderWritable;
import static app.organicmaps.sdk.util.Utils.dimen;
import static app.organicmaps.settings.BackupSettingsFragment.MAX_BACKUPS_DEFAULT_COUNT;
import static app.organicmaps.settings.BackupSettingsFragment.MAX_BACKUPS_KEY;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import androidx.annotation.NonNull;
import androidx.documentfile.provider.DocumentFile;
import app.organicmaps.R;
import app.organicmaps.sdk.util.log.Logger;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class BackupUtils
{
  private static final String BACKUP_PREFIX = "backup_";
  private static final String BACKUP_EXTENSION = ".kmz";
  private static final DateTimeFormatter DATE_FORMATTER =
      DateTimeFormatter.ofPattern("yyyy-MM-dd_HH-mm-ss").withLocale(Locale.US);
  private static final String TAG = BackupUtils.class.getSimpleName();

  public static CharSequence formatReadableFolderPath(Context context, @NonNull Uri uri)
  {
    String docId = DocumentsContract.getTreeDocumentId(uri);
    String volumeId;
    String subPath = "";

    int colonIndex = docId.indexOf(':');
    if (colonIndex >= 0)
    {
      volumeId = docId.substring(0, colonIndex);
      subPath = docId.substring(colonIndex + 1);
    }
    else
    {
      volumeId = docId;
    }

    String volumeName;
    if ("primary".equalsIgnoreCase(volumeId))
      volumeName = context.getString(app.organicmaps.sdk.R.string.maps_storage_shared);
    else
      volumeName = context.getString(app.organicmaps.sdk.R.string.maps_storage_removable);

    SpannableStringBuilder sb = new SpannableStringBuilder();
    sb.append(volumeName + ": \n", new AbsoluteSizeSpan(dimen(context, R.dimen.text_size_body_3)),
              Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    sb.append("/" + subPath, new AbsoluteSizeSpan(dimen(context, R.dimen.text_size_body_4)),
              Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return sb;
  }

  public static int getMaxBackups(SharedPreferences prefs)
  {
    String rawValue = prefs.getString(MAX_BACKUPS_KEY, String.valueOf(MAX_BACKUPS_DEFAULT_COUNT));
    try
    {
      return Integer.parseInt(rawValue);
    }
    catch (NumberFormatException e)
    {
      Logger.e(
          TAG,
          "Failed to parse max backups count, raw value: " + rawValue + " set to default: " + MAX_BACKUPS_DEFAULT_COUNT,
          e);
      prefs.edit().putString(MAX_BACKUPS_KEY, String.valueOf(MAX_BACKUPS_DEFAULT_COUNT)).apply();
      return MAX_BACKUPS_DEFAULT_COUNT;
    }
  }

  public static DocumentFile createUniqueBackupFolder(@NonNull DocumentFile parentDir, LocalDateTime backupTime)
  {
    String folderName = BACKUP_PREFIX + backupTime.format(DATE_FORMATTER);
    return parentDir.createDirectory(folderName);
  }

  public static String getBackupName(LocalDateTime backupTime)
  {
    String formattedBackupTime = backupTime.format(DATE_FORMATTER);
    return BACKUP_PREFIX + formattedBackupTime + BACKUP_EXTENSION;
  }

  public static DocumentFile[] getBackupFolders(DocumentFile parentDir)
  {
    List<DocumentFile> backupFolders = new ArrayList<>();
    for (DocumentFile file : parentDir.listFiles())
    {
      if (file.isDirectory() && file.getName() != null && file.getName().startsWith(BACKUP_PREFIX))
        backupFolders.add(file);
    }
    return backupFolders.toArray(new DocumentFile[0]);
  }

  public static boolean isBackupFolderAvailable(Context context, String storedFolderPath)
  {
    return !TextUtils.isEmpty(storedFolderPath) && isFolderWritable(context, storedFolderPath);
  }
}
