package app.organicmaps.sdk.util;

import android.util.Log;
import java.lang.reflect.Method;

public class ROMUtils
{
  private static final String TAG = "ROMUtils";

  public static boolean isCustomROM()
  {
    Method method = null;
    try
    {
      Class<?> systemProperties = Class.forName("android.os.SystemProperties");
      method = systemProperties.getMethod("get", String.class);
    }
    catch (Exception e)
    {
      Log.e(TAG, "Error getting SystemProperties: ", e);
    }

    if (method == null)
      return false;

    // Check common custom ROM properties
    String[] customROMIndicators = {
        "ro.modversion",
        "ro.cm.version", // LineageOS/CyanogenMod-specific
        "ro.lineage.build.version", // LineageOS
    };

    // Indexed loop (not foreach) on purpose: HardwareIdDetector.findReflectionUsage crashes in Lint
    // with getParameterIndex "Not my parameter" when a Method.invoke call sits inside a foreach, so the
    // loop variable must not be a foreach parameter.
    for (int i = 0; i < customROMIndicators.length; i++)
    {
      final String prop = customROMIndicators[i];
      try
      {
        String value = (String) method.invoke(null, prop);
        if (value != null && !value.isEmpty())
        {
          Log.d(TAG, "Custom ROM detected: " + prop + " = " + value);
          return true;
        }
      }
      catch (Exception e)
      {
        Log.e(TAG, "Error invoking method: ", e);
      }
    }

    return false;
  }
}
