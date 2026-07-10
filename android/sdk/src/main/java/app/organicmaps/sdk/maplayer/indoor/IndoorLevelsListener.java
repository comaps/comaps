package app.organicmaps.sdk.maplayer.indoor;

import androidx.annotation.NonNull;

public interface IndoorLevelsListener
{
  /**
   * Called when the set of indoor levels in the viewport changes.
   *
   * @param levels      formatted level labels sorted from the topmost floor down;
   *                    empty when no indoor data is visible (hide the selector).
   * @param activeLevel the currently selected level.
   */
  void onLevelsChanged(@NonNull String[] levels, @NonNull String activeLevel);
}
