package app.organicmaps.routing;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RouteStepInfo;
import app.organicmaps.sdk.util.Utils;

public class RoutePreviewController
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final MapButtonsViewModel mMapButtonsViewModel;
  @Nullable
  private RouteStepInfo[] mRouteSteps;
  private int mPreviewedTurnPosition = -1;

  public RoutePreviewController(@NonNull Context context, @NonNull MapButtonsViewModel mapButtonsViewModel)
  {
    mContext = context;
    mMapButtonsViewModel = mapButtonsViewModel;
  }

  public void onTurnPreviewChanged(boolean isPreviewing, int segmentIndex)
  {
    if (!isPreviewing)
    {
      mPreviewedTurnPosition = -1;
      mRouteSteps = null;
      mMapButtonsViewModel.setRoutePreviewControlsState(MapButtonsViewModel.RoutePreviewControlsState.none());
      return;
    }

    if (mRouteSteps == null)
      mRouteSteps = Framework.nativeGetRouteSteps(Utils.getLanguageCode());

    if (mRouteSteps == null || mRouteSteps.length == 0)
    {
      mMapButtonsViewModel.setRoutePreviewControlsState(MapButtonsViewModel.RoutePreviewControlsState.none());
      return;
    }

    // route step sequence numbers don't correspond to segment indexes so we must match it here
    mPreviewedTurnPosition = -1;
    for (int i = 0; i < mRouteSteps.length; ++i)
    {
      if (mRouteSteps[i].index == segmentIndex)
      {
        mPreviewedTurnPosition = i;
        break;
      }
    }

    if (mPreviewedTurnPosition < 0)
    {
      mMapButtonsViewModel.setRoutePreviewControlsState(MapButtonsViewModel.RoutePreviewControlsState.none());
      return;
    }

    mMapButtonsViewModel.setRoutePreviewControlsState(new MapButtonsViewModel.RoutePreviewControlsState(
        true,
        mContext.getString(R.string.route_preview_turn_position, mRouteSteps[mPreviewedTurnPosition].sequence,
                           mRouteSteps.length),
        mPreviewedTurnPosition > 0, mPreviewedTurnPosition + 1 < mRouteSteps.length));
  }

  @Nullable
  public String getPreviewedTurnInstruction()
  {
    if (mRouteSteps == null || mPreviewedTurnPosition < 0)
      return null;
    return mRouteSteps[mPreviewedTurnPosition].textualInstruction;
  }

  public void previewPreviousTurn()
  {
    previewTurnAtPosition(mPreviewedTurnPosition - 1);
  }

  public void previewNextTurn()
  {
    previewTurnAtPosition(mPreviewedTurnPosition + 1);
  }

  public void exitPreview()
  {
    Framework.nativeClearPreviewedTurn();
  }

  private void previewTurnAtPosition(int position)
  {
    if (mRouteSteps == null || position < 0 || position >= mRouteSteps.length)
      return;

    Framework.nativePreviewTurn(mRouteSteps[position].index);
  }
}
