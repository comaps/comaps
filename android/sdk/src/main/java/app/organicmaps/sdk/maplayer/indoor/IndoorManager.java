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

  private static native void nativeAddListener(@NonNull OnIndoorLevelsChangedListener listener);
  private static native void nativeRemoveListener();
  private static native void nativeSelectLevel(@NonNull String level);
  @NonNull
  private static native String nativeGetActiveLevel();
  @NonNull
  private static native String[] nativeGetViewportLevels();
}
