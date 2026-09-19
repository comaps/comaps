package app.organicmaps.crashhandler;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.util.Log;
import app.organicmaps.sdk.util.log.Logger;
import java.io.File;
import java.lang.String;
import java.lang.System;
import ru.ivanarh.jndcrash.NDCrash;
import ru.ivanarh.jndcrash.NDCrashError;
import ru.ivanarh.jndcrash.NDCrashUnwinder;
import ru.ivanarh.jndcrash.NDCrashUtils;

public class CrashHandler
{
  private static final String TAG = "CrashHandler";

  private CrashHandler() {}

  public static String getReportDir(Context context)
  {
    // If this directory is full, that's probably the cause of the crash, so no harm in losing the logs
    return context.getFilesDir().getAbsolutePath() + "/crashes";
  }

  public static boolean isCrashHandler(Context context)
  {
    return NDCrashUtils.isCrashServiceProcess(context, NativeCrashHandlerService.class);
  }

  public static void installHandlers(Context context)
  {
    try
    {
      final String reportDir = getReportDir(context);
      if (!(new File(reportDir)).mkdirs())
      {
        Log.e(TAG, "failed to mkdirs " + reportDir);
      }
      final String reportPath = reportDir + "/crash_" + System.currentTimeMillis() + "_" + android.os.Process.myPid();
      installJavaCrashHandler(context, reportPath + "_java.txt");
      installNativeCrashHandler(context, reportPath + "_native.txt");
      checkForPendingCrashes(context, reportDir);
    }
    catch (RuntimeException ex)
    {
      Log.e(TAG, "failed to install crash handlers", ex);
    }
  }

  public static void installJavaCrashHandler(Context context, String reportPath)
  {
    Thread.UncaughtExceptionHandler handler = new JavaCrashHandler(context, reportPath);
    Thread.setDefaultUncaughtExceptionHandler(handler);
  }

  public static void installNativeCrashHandler(Context context, String reportPath)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
      return;
    final NDCrashError ndError = NDCrash.initializeOutOfProcess(context, reportPath, NDCrashUnwinder.libunwindstack,
                                                                NativeCrashHandlerService.class);
    if (ndError != NDCrashError.ok)
    {
      Logger.e(TAG, "Failed to install crash handler: " + ndError);
    }
  }

  public static void checkForPendingCrashes(Context context, String reportDir)
  {
    File dir = new File(reportDir);
    if (!dir.exists() || !dir.isDirectory())
    {
      return;
    }
    String[] logs = dir.list();
    if (logs != null && logs.length > 0)
    {
      Intent i = new Intent(context, CrashHandlerActivity.class);
      i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(i);
    }
  }
}
