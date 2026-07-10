package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

class OnIndoorLevelsChangedListener
{
  @Nullable
  private IndoorLevelsListener mListener;

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void onLevelsChanged(@NonNull String[] levels, @NonNull String activeLevel)
  {
    if (mListener == null)
      return;
    mListener.onLevelsChanged(levels, activeLevel);
  }

  public void attach(@NonNull IndoorLevelsListener listener)
  {
    mListener = listener;
  }

  public void detach()
  {
    mListener = null;
  }
}
