package app.organicmaps.routing;

import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;

public class RoutePreviewPanelController
{
  @NonNull
  private final View mRoutingPlanFrame;
  @NonNull
  private final View mRoutePreviewFrame;
  @NonNull
  private final View mRoutePreviewToolbar;
  @NonNull
  private final TextView mInstruction;
  @NonNull
  private final Runnable mOnClose;

  public RoutePreviewPanelController(@NonNull View root, @NonNull Runnable onClose)
  {
    mOnClose = onClose;
    mRoutingPlanFrame = root.findViewById(R.id.routing_plan_frame);
    mRoutePreviewFrame = root.findViewById(R.id.route_preview_frame);
    mRoutePreviewToolbar = mRoutePreviewFrame.findViewById(R.id.route_preview_toolbar);
    mInstruction = mRoutePreviewFrame.findViewById(R.id.route_preview_instruction_text);

    ViewCompat.setOnApplyWindowInsetsListener(mRoutePreviewToolbar, PaddingInsetsListener.excludeBottom());
    mRoutePreviewFrame.findViewById(R.id.route_preview_back).setOnClickListener(v -> mOnClose.run());
  }

  public void onTurnPreviewChanged(boolean isPreviewing, @Nullable String instruction)
  {
    if (isPreviewing)
    {
      mInstruction.setText(instruction);
      mRoutePreviewFrame.setVisibility(View.VISIBLE);
      mRoutingPlanFrame.setVisibility(View.GONE);
      ViewCompat.requestApplyInsets(mRoutePreviewToolbar);
      return;
    }

    mRoutePreviewFrame.setVisibility(View.GONE);
    mRoutingPlanFrame.setVisibility(RoutingController.get().isPlanning() ? View.VISIBLE : View.INVISIBLE);
  }
}
