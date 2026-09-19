package app.organicmaps.crashhandler;

import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;

import android.content.Intent;
import android.util.Log;
import java.lang.String;
import ru.ivanarh.jndcrash.NDCrashService;

/**
 * This class runs in a separate process which may have no Looper.
 */
public class NativeCrashHandlerService extends NDCrashService
{
  private static final String TAG = NativeCrashHandlerService.class.getSimpleName();

  @Override
  public void onCrash(String reportPath)
  {
    Log.e("crash!", reportPath);
    // try to handle the report immediately, if this fails we will still do it on next launch in CrashHandler
    try
    {
      Intent i = new Intent(this, CrashHandlerActivity.class);
      i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      startActivity(i);
    }
    catch (RuntimeException ex)
    {
      Log.e(TAG, "failed to launch crash handler, deferring...", ex);
    }
  }
}
