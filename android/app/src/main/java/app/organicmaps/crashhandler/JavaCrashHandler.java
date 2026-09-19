package app.organicmaps.crashhandler;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import androidx.annotation.NonNull;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;

public class JavaCrashHandler implements Thread.UncaughtExceptionHandler
{
  private static final String TAG = JavaCrashHandler.class.getSimpleName();

  private final Context context;
  private final String reportPath;

  public JavaCrashHandler(Context context, String reportPath)
  {
    this.context = context;
    this.reportPath = reportPath;
  }

  @Override
  public void uncaughtException(@NonNull Thread t, @NonNull Throwable e)
  {
    try
    {
      File report = new File(reportPath);
      try (BufferedWriter fileWriter = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(report))))
      {
        fileWriter.write(Log.getStackTraceString(e));
      }

      Intent i = new Intent(context, CrashHandlerActivity.class);
      i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      context.startActivity(i);
    }
    catch (Throwable ex)
    {
      Log.e(TAG, "failed to report an unhandled exception", ex);
    }
  }
}
