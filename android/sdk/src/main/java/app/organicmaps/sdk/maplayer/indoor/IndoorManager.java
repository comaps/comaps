package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.NonNull;

public class IndoorManager
{
  @NonNull
  private final OnIndoorLevelsChangedListener mListener = new OnIndoorLevelsChangedListener();

  public void initialize()
  {
    nativeAddListener(mListener);
  }

  public void attach(@NonNull IndoorLevelsListener listener)
  {
    mListener.attach(listener);
  }

  public void detach()
  {
    mListener.detach();
  }

  public static void selectLevel(@NonNull String level)
  {
    nativeSelectLevel(level);
  }

  @NonNull
  public static String getActiveLevel()
  {
    return nativeGetActiveLevel();
  }

  // Current viewport levels (topmost floor first), or empty when indoor mode is inactive. Lets a
  // freshly created UI re-sync without waiting for the next levels-changed notification.
  @NonNull
  public static String[] getViewportLevels()
  {
    return nativeGetViewportLevels();
  }

  public static boolean isDebugEnabled()
  {
    return nativeIsDebugEnabled();
  }

  // Returns a human-readable description of the feature that most recently triggered indoor mode,
  // or empty string if indoor mode hasn't been entered yet.
  @NonNull
  public static String getActivatingInfo()
  {
    return nativeGetActivatingInfo();
  }

  // Returns a human-readable description of the debug indoor feature nearest to the given screen
  // pixel coordinate, or an empty string if debug mode is off or nothing is nearby.
  @NonNull
  public static String getDebugFeatureAt(int screenX, int screenY)
  {
    return nativeGetDebugFeatureAt(screenX, screenY);
  }

  private static native void nativeAddListener(@NonNull OnIndoorLevelsChangedListener listener);
  private static native void nativeRemoveListener();
  private static native void nativeSelectLevel(@NonNull String level);
  @NonNull
  private static native String nativeGetActiveLevel();
  @NonNull
  private static native String[] nativeGetViewportLevels();
  private static native boolean nativeIsDebugEnabled();
  @NonNull
  private static native String nativeGetActivatingInfo();
  @NonNull
  private static native String nativeGetDebugFeatureAt(int screenX, int screenY);
}
