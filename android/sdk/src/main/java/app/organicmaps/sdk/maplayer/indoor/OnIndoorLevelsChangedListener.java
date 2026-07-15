package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

class OnIndoorLevelsChangedListener
{
  @Nullable
  private IndoorLevelsListener mListener;

  @Nullable
  private String[] mLastLevels;
  @Nullable
  private String mLastActiveLevel;

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void onLevelsChanged(@NonNull String[] levels, @NonNull String activeLevel)
  {
    mLastLevels = levels;
    mLastActiveLevel = activeLevel;
    if (mListener == null)
      return;
    mListener.onLevelsChanged(levels, activeLevel);
  }

  public void attach(@NonNull IndoorLevelsListener listener)
  {
    mListener = listener;
    // Replay the latest state so a listener attached after the last change is up to date.
    if (mLastLevels != null && mLastActiveLevel != null)
      listener.onLevelsChanged(mLastLevels, mLastActiveLevel);
  }

  public void detach()
  {
    mListener = null;
  }
}
