package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.NonNull;
import java.text.NumberFormat;

public class IndoorManager
{
  @NonNull
  private final OnIndoorLevelsChangedListener mListener = new OnIndoorLevelsChangedListener();

  public void initialize()
  {
    nativeAddListener(mListener);
  }

  public void setLevelsListener(@NonNull IndoorLevelsListener listener)
  {
    mListener.setListener(listener);
  }

  public void removeLevelsListener()
  {
    mListener.setListener(null);
  }

  /**
   * @param level a floor of the active complex, as reported by getViewportLevels
   * @return false when no complex is active or the complex has no such floor
   */
  public static boolean selectLevel(double level)
  {
    return nativeSelectLevel(level);
  }

  @NonNull
  public static double[] getViewportLevels()
  {
    return nativeGetViewportLevels();
  }

  public static double getActiveLevel()
  {
    return nativeGetActiveLevel();
  }

  /**
   * @param level a floor number, half-steps included
   * @return the floor as the device locale writes it, e.g. "-1" or "1,5"
   */
  @NonNull
  public static String formatLevel(double level)
  {
    final NumberFormat format = NumberFormat.getNumberInstance();
    format.setMinimumFractionDigits(0);
    format.setMaximumFractionDigits(1);
    return format.format(level);
  }

  private static native void nativeAddListener(@NonNull OnIndoorLevelsChangedListener listener);
  private static native void nativeRemoveListener();
  private static native boolean nativeSelectLevel(double level);
  @NonNull
  private static native double[] nativeGetViewportLevels();
  private static native double nativeGetActiveLevel();
}
