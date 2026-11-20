package app.organicmaps.routing;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RoutePlan;
import app.organicmaps.sdk.util.Utils;

public class RoutePreviewController
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final MapButtonsViewModel mMapButtonsViewModel;
  @Nullable
  private RoutePlan mRoutePlan;
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
      mRoutePlan = null;
      mMapButtonsViewModel.setRoutePreviewControlsState(MapButtonsViewModel.RoutePreviewControlsState.none());
      return;
    }

    if (mRoutePlan == null)
      mRoutePlan = Framework.nativeGetRoutePlan(Utils.getLanguageCode());

    if (mRoutePlan == null || mRoutePlan.steps.length == 0)
    {
      mMapButtonsViewModel.setRoutePreviewControlsState(MapButtonsViewModel.RoutePreviewControlsState.none());
      return;
    }

    // route step sequence numbers don't correspond to segment indexes so we must match it here
    mPreviewedTurnPosition = -1;
    for (int i = 0; i < mRoutePlan.steps.length; ++i)
    {
      if (mRoutePlan.steps[i].index == segmentIndex)
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
        mContext.getString(R.string.route_preview_turn_position,
                           mRoutePlan.steps[mPreviewedTurnPosition].sequence + 1,
                           mRoutePlan.steps.length),
        mPreviewedTurnPosition > 0, mPreviewedTurnPosition + 1 < mRoutePlan.steps.length));
  }

  @Nullable
  public String getPreviewedTurnInstruction()
  {
    if (mRoutePlan == null || mPreviewedTurnPosition < 0)
      return null;
    return mRoutePlan.steps[mPreviewedTurnPosition].textualInstruction;
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
    if (mRoutePlan == null || position < 0 || position >= mRoutePlan.steps.length)
      return;

    Framework.nativePreviewTurn(mRoutePlan.steps[position].index);
  }
}
