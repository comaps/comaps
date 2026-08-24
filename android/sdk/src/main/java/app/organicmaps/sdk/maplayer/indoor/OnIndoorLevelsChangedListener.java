package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

// Called from JNI on the UI thread.
public class OnIndoorLevelsChangedListener
{
  @Nullable
  private IndoorLevelsListener mListener;

  public void setListener(@Nullable IndoorLevelsListener listener)
  {
    mListener = listener;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void onLevelsChanged(@NonNull double[] levels, double activeLevel)
  {
    if (mListener != null)
      mListener.onIndoorLevelsChanged(levels, activeLevel);
  }
}
