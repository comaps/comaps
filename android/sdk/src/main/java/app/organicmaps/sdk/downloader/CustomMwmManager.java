package app.organicmaps.sdk.downloader;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.log.Logger;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Manages custom MWM map files that are manually imported by the user.
 * Custom maps are stored in dated folders (YYMMDD format) and take precedence
 * over downloaded maps when loading.
 */
public class CustomMwmManager
{
  private static final String TAG = CustomMwmManager.class.getSimpleName();
  private static final String CUSTOM_MAPS_DIR = "custom_maps";
  private static final String MWM_EXTENSION = ".mwm";

  public enum ImportResult
  {
    SUCCESS,
    ERROR_INVALID_FILE,
    ERROR_IO,
    ERROR_STORAGE
  }

  /**
   * Represents a custom MWM file with its metadata.
   */
  public static class CustomMwmFile
  {
    public final String name;       // e.g., "Portland"
    public final String path;       // Full path to the file
    public final long version;      // Date version as YYMMDD number
    public final long fileSize;

    public CustomMwmFile(String name, String path, long version, long fileSize)
    {
      this.name = name;
      this.path = path;
      this.version = version;
      this.fileSize = fileSize;
    }
  }

  /**
   * Gets the custom maps root directory path.
   */
  @NonNull
  public static String getCustomMapsDir(@NonNull Context context)
  {
    String writableDir = Framework.nativeGetWritableDir();
    return StorageUtils.addTrailingSeparator(writableDir) + CUSTOM_MAPS_DIR;
  }

  /**
   * Gets or creates the custom maps directory for today's date.
   * @return The directory path, or null if creation failed.
   */
  @Nullable
  public static String getTodayCustomMapsDir(@NonNull Context context)
  {
    //TODO: Ideally the MWM creation date would be baked into the file, not assumed
    String customMapsDir = getCustomMapsDir(context);
    String today = new SimpleDateFormat("yyMMdd", Locale.US).format(new Date());
    String todayDir = StorageUtils.addTrailingSeparator(customMapsDir) + today;

    if (!StorageUtils.createDirectory(todayDir))
      return null;

    return todayDir;
  }

  /**
   * Imports an MWM file from a content URI into the custom maps directory.
   * The file is saved in a dated folder based on today's date.
   *
   * @param context Application context
   * @param uri     Content URI of the MWM file
   * @return ImportResult indicating success or the type of error
   */
  @NonNull
  public static ImportResult importMwmFile(@NonNull Context context, @NonNull Uri uri)
  {
    ContentResolver resolver = context.getContentResolver();
    String fileName = StorageUtils.getFileNameFromUri(resolver, uri);
    if (fileName == null || !fileName.toLowerCase(Locale.US).endsWith(MWM_EXTENSION))
    {
      Logger.e(TAG, "Invalid file name or not an MWM file: " + fileName);
      return ImportResult.ERROR_INVALID_FILE;
    }

    // Strip duplicate-download suffixes added by browsers/file managers (e.g. "Portland (2).mwm" -> "Portland.mwm")
    fileName = fileName.replaceAll("(?i)\\s*\\(\\d+\\)(\\.mwm)$", "$1");

    // Sanity-check: MWM files are a FilesContainer whose first 8 bytes are a little-endian uint64_t
    // pointing to the table of contents. If those bytes are 0 or implausibly large the file is junk.
    if (!isValidMwmFile(resolver, uri))
    {
      Logger.e(TAG, "File does not appear to be a valid MWM container: " + fileName);
      return ImportResult.ERROR_INVALID_FILE;
    }

    String destDir = getTodayCustomMapsDir(context);
    if (destDir == null)
    {
      return ImportResult.ERROR_STORAGE;
    }

    File destFile = new File(destDir, fileName);
    Logger.i(TAG, "Importing MWM file to: " + destFile.getAbsolutePath());

    try
    {
      if (!StorageUtils.copyFile(resolver, uri, destFile))
      {
        Logger.e(TAG, "Failed to copy MWM file");
        return ImportResult.ERROR_IO;
      }

      Logger.i(TAG, "Successfully imported MWM file: " + fileName);
      return ImportResult.SUCCESS;
    }
    catch (IOException e)
    {
      Logger.e(TAG, "IOException while importing MWM file", e);
      return ImportResult.ERROR_IO;
    }
  }

  /**
   * Lists all custom MWM files, grouped by map name with the newest version taking precedence.
   * @return Map from country name to CustomMwmFile (newest version)
   */
  @NonNull
  public static Map<String, CustomMwmFile> getCustomMwmFiles(@NonNull Context context)
  {
    Map<String, CustomMwmFile> result = new HashMap<>();
    String customMapsDir = getCustomMapsDir(context);
    File rootDir = new File(customMapsDir);

    if (!rootDir.exists() || !rootDir.isDirectory())
    {
      return result;
    }

    File[] versionDirs = rootDir.listFiles(File::isDirectory);
    if (versionDirs == null)
    {
      return result;
    }

    // Sort by version descending (newest first)
    Arrays.sort(versionDirs, (a, b) -> {
      long versionA = parseVersion(a.getName());
      long versionB = parseVersion(b.getName());
      return Long.compare(versionB, versionA);
    });

    for (File versionDir : versionDirs)
    {
      long version = parseVersion(versionDir.getName());
      if (version <= 0)
      {
        continue;
      }

      File[] mwmFiles = versionDir.listFiles((dir, name) ->
          name.toLowerCase(Locale.US).endsWith(MWM_EXTENSION));

      if (mwmFiles == null)
      {
        continue;
      }

      for (File mwmFile : mwmFiles)
      {
        String name = mwmFile.getName();
        // Remove .mwm extension to get country name
        String countryName = name.substring(0, name.length() - MWM_EXTENSION.length());

        // Only add if we don't already have a newer version
        if (!result.containsKey(countryName))
        {
          result.put(countryName, new CustomMwmFile(
              countryName,
              mwmFile.getAbsolutePath(),
              version,
              mwmFile.length()
          ));
        }
      }
    }

    return result;
  }

  /**
   * Returns true if an officially-downloaded (non-custom) copy of the map exists on disk.
   * This can be true even when a custom map is active (shadowing it).
   */
  public static boolean hasOfficialMapOnDisk(@NonNull Context context, @NonNull String countryName)
  {
    for (File versionDir : getOfficialVersionDirs())
      if (new File(versionDir, countryName + MWM_EXTENSION).exists())
        return true;
    return false;
  }

  /**
   * Checks if a custom version of a map exists.
   * @param countryName The country/map name (without .mwm extension)
   * @return The CustomMwmFile if found, null otherwise
   */
  @Nullable
  public static CustomMwmFile getCustomMwmFile(@NonNull Context context, @NonNull String countryName)
  {
    Map<String, CustomMwmFile> customFiles = getCustomMwmFiles(context);
    return customFiles.get(countryName);
  }

  /**
   * Deletes the officially-downloaded (non-custom) copy of a map from disk.
   * The custom map, if present, is unaffected. Call nativeReloadWorldMaps() after this.
   */
  public static void deleteOfficialMapFromDisk(@NonNull Context context, @NonNull String countryName)
  {
    for (File versionDir : getOfficialVersionDirs())
    {
      File mwmFile = new File(versionDir, countryName + MWM_EXTENSION);
      if (mwmFile.exists())
      {
        Logger.i(TAG, "Deleting official map: " + mwmFile.getAbsolutePath());
        if (!mwmFile.delete())
          Logger.e(TAG, "Failed to delete official map: " + mwmFile.getAbsolutePath());
      }
    }
  }

  /**
   * Returns all numeric (official) version directories under the writable dir,
   * excluding the custom_maps folder.
   */
  @NonNull
  private static File[] getOfficialVersionDirs()
  {
    File root = new File(StorageUtils.addTrailingSeparator(Framework.nativeGetWritableDir()));
    File[] dirs = root.listFiles(f -> f.isDirectory() && f.getName().matches("\\d+")
                                      && !f.getName().equals(CUSTOM_MAPS_DIR));
    return dirs != null ? dirs : new File[0];
  }

  /**
   * Deletes a custom MWM file.
   * @param context Application context
   * @param file The CustomMwmFile to delete
   * @return true if deletion was successful
   */
  public static boolean deleteCustomMwmFile(@NonNull Context context, @NonNull CustomMwmFile file)
  {
    File f = new File(file.path);
    boolean deleted = f.delete();

    if (deleted)
    {
      Logger.i(TAG, "Deleted custom MWM file: " + file.path);
      // Clean up empty version directories
      File customMapsDir = new File(getCustomMapsDir(context));
      StorageUtils.removeEmptyDirectories(customMapsDir);
    }
    else
    {
      Logger.e(TAG, "Failed to delete custom MWM file: " + file.path);
    }

    return deleted;
  }

  /**
   * Checks whether the URI looks like a valid MWM FilesContainer by reading the first 8 bytes.
   * A FilesContainer starts with a little-endian uint64_t that is the offset of the TOC.
   * The offset must be > 0 and less than the file's declared size (if determinable).
   */
  private static boolean isValidMwmFile(@NonNull ContentResolver resolver, @NonNull Uri uri)
  {
    try (InputStream is = resolver.openInputStream(uri))
    {
      if (is == null)
        return false;

      byte[] header = new byte[8];
      int read = 0;
      while (read < 8)
      {
        int n = is.read(header, read, 8 - read);
        if (n < 0)
          return false;
        read += n;
      }

      long tocOffset = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN).getLong();
      // TOC offset must be positive; reject obviously bogus values (> 8 GB is unrealistic for a map file)
      return tocOffset > 0 && tocOffset < 8L * 1024 * 1024 * 1024;
    }
    catch (IOException e)
    {
      return false;
    }
  }

  /**
   * Parses a version string (YYMMDD format) to a long.
   * @return The version number, or 0 if parsing failed
   */
  private static long parseVersion(String versionStr)
  {
    if (versionStr == null || versionStr.length() != 6)
    {
      return 0;
    }

    try
    {
      return Long.parseLong(versionStr);
    }
    catch (NumberFormatException e)
    {
      return 0;
    }
  }

}
