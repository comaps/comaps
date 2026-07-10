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

  private static native void nativeAddListener(@NonNull OnIndoorLevelsChangedListener listener);
  private static native void nativeRemoveListener();
  private static native void nativeSelectLevel(@NonNull String level);
  @NonNull
  private static native String nativeGetActiveLevel();
}
