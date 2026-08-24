package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.NonNull;

public interface IndoorLevelsListener
{
  // Floors top first; empty means the picker should hide.
  void onIndoorLevelsChanged(@NonNull double[] levels, double activeLevel);
}
