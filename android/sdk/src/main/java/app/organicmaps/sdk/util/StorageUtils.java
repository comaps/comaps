package app.organicmaps.sdk.util;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.FileProvider;
import androidx.documentfile.provider.DocumentFile;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;

public class StorageUtils
{
  private static final String TAG = StorageUtils.class.getSimpleName();

  public static boolean isDirWritable(File dir)
  {
    final String path = dir.getPath();
    Logger.d(TAG, "Checking for writability " + path);

    // Its better to be conservative here and don't allow to use the storage
    // if any of the system calls behave unexpectedly,
    // still we want extra logging to facilitate debugging possible fringe cases,
    // e.g. https://github.com/organicmaps/organicmaps/issues/2684
    boolean success = true;
    if (!dir.isDirectory())
    {
      Logger.w(TAG, "Not a directory: " + path);
      success = false;
    }
    if (!dir.exists())
    {
      Logger.w(TAG, "Not exists: " + path);
      success = false;
    }
    if (!dir.canWrite())
    {
      Logger.w(TAG, "Not writable: " + path);
      success = false;
    }
    if (!dir.canRead())
    {
      Logger.w(TAG, "Not readable: " + path);
      success = false;
    }
    if (dir.list() == null)
    {
      Logger.w(TAG, "Not listable: " + path);
      success = false;
    }

    final File newDir = new File(dir, "om_test_dir");
    final String newPath = newDir.getPath();
    if (newDir.delete())
      Logger.i(TAG, "Deleting existing test file/dir: " + newPath);
    if (newDir.exists())
      Logger.w(TAG, "Existing test file/dir is not deleted (not empty?): " + newPath);
    if (!newDir.mkdir())
    {
      Logger.w(TAG, "Failed to create the test dir: " + newPath);
      success = false;
    }
    if (!newDir.exists())
    {
      Logger.w(TAG, "The test dir doesn't exist: " + newPath);
      success = false;
    }
    if (!newDir.delete())
    {
      Logger.w(TAG, "Failed to delete the test dir: " + newPath);
      success = false;
    }

    return success;
  }

  @NonNull
  public static String getApkPath(@NonNull Context context)
  {
    try
    {
      return Utils.getApplicationInfo(context.getPackageManager(), Config.getApplicationId(), 0).sourceDir;
    }
    catch (final PackageManager.NameNotFoundException e)
    {
      Logger.e(TAG, "Can't get apk path from PackageManager", e);
      return "";
    }
  }

  @NonNull
  public static String addTrailingSeparator(@NonNull String dir)
  {
    if (!dir.endsWith(File.separator))
      return dir + File.separator;
    return dir;
  }

  @NonNull
  public static String getSettingsPath(@NonNull Context context)
  {
    return addTrailingSeparator(context.getFilesDir().getAbsolutePath());
  }

  @NonNull
  public static String getPrivatePath(@NonNull Context context)
  {
    return addTrailingSeparator(context.getFilesDir().getAbsolutePath());
  }

  @NonNull
  public static String getTempPath(@NonNull Context context)
  {
    return addTrailingSeparator(context.getCacheDir().getAbsolutePath());
  }

  public static boolean createDirectory(@NonNull final String path)
  {
    final File directory = new File(path);
    if (!directory.exists() && !directory.mkdirs())
    {
      final String errMsg = "Can't create directory " + path;
      Logger.e(TAG, errMsg);
      return false;
    }
    return true;
  }

  public static void requireDirectory(@Nullable final String path) throws IOException
  {
    if (!createDirectory(path))
      throw new IOException("Can't create directory " + path);
  }

  @NonNull
  public static Uri getUriForFilePath(@NonNull Context context, @NonNull String path)
  {
    return FileProvider.getUriForFile(context.getApplicationContext(), Config.getFileProviderAuthority(),
                                      new File(path));
  }

  static private boolean copyFile(InputStream from, OutputStream to) throws IOException
  {
    if (from == null || to == null)
      return false;

    byte[] buf = new byte[4 * 1024];
    int len;
    while ((len = from.read(buf)) > 0)
      to.write(buf, 0, len);

    return true;
  }

  /**
   * Copy data from a URI into a local file.
   * @param resolver content resolver
   * @param from a source URI.
   * @param to a destination file
   * @return true on success and false if the provider recently crashed.
   * @throws IOException - if I/O error occurs.
   */
  public static boolean copyFile(@NonNull ContentResolver resolver, @NonNull Uri from, @NonNull File to)
      throws IOException
  {
    try (InputStream in = resolver.openInputStream(from))
    {
      try (OutputStream out = new FileOutputStream(to))
      {
        return copyFile(in, out);
      }
    }
  }

  public static boolean copyFile(@NonNull ContentResolver resolver, @NonNull Uri from, @NonNull Uri to)
      throws IOException
  {
    try (InputStream in = resolver.openInputStream(from))
    {
      try (OutputStream out = resolver.openOutputStream(to))
      {
        return copyFile(in, out);
      }
    }
  }

  /**
   * Extracts the file name from a content or file URI.
   * @param resolver content resolver
   * @param uri the URI to extract the file name from
   * @return the file name, or null if it could not be determined
   */
  @Nullable
  public static String getFileNameFromUri(@NonNull ContentResolver resolver, @NonNull Uri uri)
  {
    String fileName = null;
    final String scheme = uri.getScheme();

    if (ContentResolver.SCHEME_CONTENT.equals(scheme))
    {
      try (Cursor cursor = resolver.query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null))
      {
        if (cursor != null && cursor.moveToFirst())
        {
          final int columnIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
          if (columnIndex >= 0)
            fileName = cursor.getString(columnIndex);
        }
      }
      catch (Exception e)
      {
        Logger.e(TAG, "Failed to get file name from content URI", e);
      }
    }

    if (fileName == null)
    {
      // Fall back to extracting from the path
      String path = uri.getPath();
      if (path != null)
      {
        int lastSlash = path.lastIndexOf('/');
        fileName = lastSlash >= 0 ? path.substring(lastSlash + 1) : path;
      }
    }

    return fileName;
  }

  /**
   * Recursively lists all movable files in the directory.
   */
  public static void listFilesRecursively(File dir, String prefix, FilenameFilter filter, ArrayList<String> relPaths)
  {
    File[] list = dir.listFiles();
    if (list == null)
    {
      Logger.w(TAG, "listFilesRecursively listFiles() returned null for " + dir.getPath());
      return;
    }

    for (File file : list)
    {
      if (file.isDirectory())
      {
        listFilesRecursively(file, prefix + file.getName() + File.separator, filter, relPaths);
        continue;
      }
      String name = file.getName();
      if (filter.accept(dir, name))
        relPaths.add(prefix + name);
    }
  }

  /**
   * Returns 0 in case of the error or if no files have passed the filter.
   */
  public static long getDirSizeRecursively(File dir, FilenameFilter fileFilter)
  {
    final File[] list = dir.listFiles();
    if (list == null)
    {
      Logger.w(TAG, "getDirSizeRecursively listFiles() returned null for " + dir.getPath());
      return 0;
    }

    long dirSize = 0;
    for (File child : list)
    {
      if (child.isDirectory())
        dirSize += getDirSizeRecursively(child, fileFilter);
      else if (fileFilter.accept(dir, child.getName()))
        dirSize += child.length();
    }
    return dirSize;
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  public static void removeEmptyDirectories(File dir)
  {
    final File[] list = dir.listFiles();
    if (list == null)
      return;
    for (File file : list)
    {
      if (!file.isDirectory())
        continue;
      removeEmptyDirectories(file);
      file.delete();
    }
  }

  @SuppressWarnings("ResultOfMethodCallIgnored")
  public static boolean removeFilesInDirectory(File dir, File[] files)
  {
    try
    {
      for (File file : files)
      {
        if (file != null)
          file.delete();
      }
      removeEmptyDirectories(dir);
      return true;
    }
    catch (Exception e)
    {
      e.printStackTrace();
      return false;
    }
  }

  @FunctionalInterface
  public interface UriVisitor {
    void visit(Uri uri);
  }

  /**
   * Recursive lists all files in the given URI.
   * @param contentResolver contentResolver instance
   * @param rootUri root URI to scan
   */
  public static void listContentProviderFilesRecursively(ContentResolver contentResolver, Uri rootUri,
                                                         UriVisitor filter)
  {
    Uri rootDir =
        DocumentsContract.buildChildDocumentsUriUsingTree(rootUri, DocumentsContract.getTreeDocumentId(rootUri));
    Queue<Uri> directories = new LinkedBlockingQueue<>();
    directories.add(rootDir);
    while (!directories.isEmpty())
    {
      Uri dir = directories.remove();

      try (Cursor cur = contentResolver.query(dir,
                                              new String[] {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                                                            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                                                            DocumentsContract.Document.COLUMN_MIME_TYPE},
                                              null, null, null))
      {
        while (cur.moveToNext())
        {
          final String docId = cur.getString(0);
          final String name = cur.getString(1);
          final String mime = cur.getString(2);
          Logger.d(TAG, "docId: " + docId + ", name: " + name + ", mime: " + mime);

          if (mime.equals(DocumentsContract.Document.MIME_TYPE_DIR))
          {
            final Uri uri = DocumentsContract.buildChildDocumentsUriUsingTree(rootUri, docId);
            directories.add(uri);
          }
          else
          {
            final Uri uri = DocumentsContract.buildDocumentUriUsingTree(rootUri, docId);
            filter.visit(uri);
          }
        }
      }
    }
  }

  public static boolean copyFileToDocumentFile(@NonNull Activity activity, @NonNull File sourceFile,
                                               @NonNull DocumentFile targetFile)
  {
    try (InputStream in = new FileInputStream(sourceFile);
         OutputStream out = activity.getContentResolver().openOutputStream(targetFile.getUri()))
    {
      if (out == null)
      {
        Logger.e(TAG, "Failed to open output stream for " + targetFile.getUri());
        return false;
      }

      byte[] buffer = new byte[8192];
      int length;

      while ((length = in.read(buffer)) > 0)
        out.write(buffer, 0, length);

      out.flush();
      return true;
    }
    catch (IOException e)
    {
      Logger.e(TAG, "Failed to copy file from " + sourceFile.getAbsolutePath() + " to " + targetFile.getUri(), e);
      return false;
    }
  }

  public static void deleteDirectoryRecursive(@NonNull DocumentFile dir)
  {
    try
    {
      for (DocumentFile file : dir.listFiles())
      {
        if (file.isDirectory())
          deleteDirectoryRecursive(file);
        else
          file.delete();
      }
      dir.delete();
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to delete directory: " + dir.getUri(), e);
    }
  }

  public static boolean isFolderWritable(Context context, String folderPath)
  {
    try
    {
      Uri folderUri = Uri.parse(folderPath);
      DocumentFile folder = DocumentFile.fromTreeUri(context, folderUri);
      if (folder != null && folder.canWrite())
      {
        DocumentFile tempFile = folder.createFile("application/octet-stream", "temp_file");
        if (tempFile != null)
        {
          tempFile.delete();
          return true;
        }
      }
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to check if folder is writable: " + folderPath, e);
    }
    return false;
  }
}
