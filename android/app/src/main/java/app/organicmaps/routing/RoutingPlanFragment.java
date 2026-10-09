package app.organicmaps.routing;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;

public class RoutingPlanFragment extends BaseMwmFragment
{
  private RoutingPlanController mPlanController;
  private RoutePreviewPanelController mRoutePreviewPanelController;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    final MwmActivity activity = (MwmActivity) requireActivity();
    View res = inflater.inflate(R.layout.fragment_routing, container, false);
    mPlanController =
        new RoutingPlanController(res, activity, activity.startRoutingOptionsForResult, activity, activity);
    mRoutePreviewPanelController = new RoutePreviewPanelController(res, activity::exitRoutePreview);

    onTurnPreviewChanged(RoutingController.get().isPreviewingTurn(), activity.getPreviewedTurnInstruction());
    return res;
  }

  public void updateBuildProgress(int progress, Router router)
  {
    mPlanController.updateBuildProgress(progress, router);
  }

  public void onTurnPreviewChanged(boolean isPreviewing, @Nullable String instruction)
  {
    if (mPlanController != null)
      mPlanController.onTurnPreviewChanged(isPreviewing);
    if (mRoutePreviewPanelController != null)
      mRoutePreviewPanelController.onTurnPreviewChanged(isPreviewing, instruction);
  }

  @Override
  public boolean onBackPressed()
  {
    if (RoutingController.get().isPreviewingTurn())
    {
      ((MwmActivity) requireActivity()).exitRoutePreview();
      return true;
    }
    return RoutingController.get().cancel();
  }

  public void restoreRoutingPanelState(@NonNull Bundle state)
  {
    mPlanController.restoreRoutingPanelState(state);
    onTurnPreviewChanged(RoutingController.get().isPreviewingTurn(),
                         ((MwmActivity) requireActivity()).getPreviewedTurnInstruction());
  }

  public void saveRoutingPanelState(@NonNull Bundle outState)
  {
    mPlanController.saveRoutingPanelState(outState);
  }

  public void showAddStartFrame()
  {
    mPlanController.showAddStartFrame();
  }

  public void showAddFinishFrame()
  {
    mPlanController.showAddFinishFrame();
  }
}
