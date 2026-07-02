package app.organicmaps.routing;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.settings.RoutingOptionsActivity;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import app.organicmaps.widget.RoutingToolbarButton;
import app.organicmaps.widget.ToolbarController;
import app.organicmaps.widget.WheelProgressView;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.textview.MaterialTextView;

public class RoutingPlanController extends ToolbarController
{
  private static final String BUNDLE_HAS_DRIVING_OPTIONS_VIEW = "has_driving_options_view";

  private final View mFrame;
  @NonNull
  private final RoutingPlanInplaceController.RoutingPlanListener mRoutingPlanListener;
  private final RadioGroup mRouterTypes;
  @NonNull
  private final WheelProgressView mProgressVehicle;
  @NonNull
  private final WheelProgressView mProgressPedestrian;
  @NonNull
  private final WheelProgressView mProgressTransit;
  @NonNull
  private final WheelProgressView mProgressBicycle;
  @NonNull
  private final WheelProgressView mProgressRuler;

  @NonNull
  private final RoutingBottomMenuController mRoutingBottomMenuController;

  int mFrameHeight;
  final int mAnimToggle;

  @NonNull
  private final FrameLayout mRoutingOptionsBanner;

  @NonNull
  private final View.OnLayoutChangeListener mRoutingOptionsLayoutListener;

  private void setupRouterButton(@IdRes int buttonId, final @DrawableRes int iconRes,
                                 View.OnClickListener clickListener)
  {
    CompoundButton.OnCheckedChangeListener listener = (buttonView, isChecked) ->
    {
      RoutingToolbarButton button = (RoutingToolbarButton) buttonView;
      button.setIcon(iconRes);
      if (isChecked)
        button.activate();
      else
        button.deactivate();
    };

    RoutingToolbarButton rb = mRouterTypes.findViewById(buttonId);
    listener.onCheckedChanged(rb, false);
    rb.setOnCheckedChangeListener(listener);
    rb.setOnClickListener(clickListener);
  }

  RoutingPlanController(View root, Activity activity, ActivityResultLauncher<Intent> startRoutingOptionsForResult,
                        @NonNull RoutingPlanInplaceController.RoutingPlanListener routingPlanListener,
                        @NonNull RoutingBottomMenuListener listener)
  {
    super(root, activity);
    mFrame = root;
    mRoutingPlanListener = routingPlanListener;

    mRouterTypes = getToolbar().findViewById(R.id.route_type);

    setupRouterButtons();

    View progressFrame = getToolbar().findViewById(R.id.progress_frame);
    mProgressVehicle = progressFrame.findViewById(R.id.progress_vehicle);
    mProgressPedestrian = progressFrame.findViewById(R.id.progress_pedestrian);
    mProgressTransit = progressFrame.findViewById(R.id.progress_transit);
    mProgressBicycle = progressFrame.findViewById(R.id.progress_bicycle);
    mProgressRuler = progressFrame.findViewById(R.id.progress_ruler);
    //    mProgressTaxi = (WheelProgressView) progressFrame.findViewById(R.id.progress_taxi);

    mRoutingBottomMenuController = RoutingBottomMenuController.newInstance(requireActivity(), mFrame, listener);

    mRoutingOptionsBanner = mFrame.findViewById(R.id.routing_options_banner);
    View btn = mFrame.findViewById(R.id.routing_options_btn);
    btn.setOnClickListener(v -> RoutingOptionsActivity.start(requireActivity(), startRoutingOptionsForResult));

    mRoutingOptionsLayoutListener = new SelfTerminatedRoutingOptionsLayoutListener();
    mAnimToggle =
        MwmApplication.from(activity.getApplicationContext()).getResources().getInteger(R.integer.anim_default);

    final View menuFrame = activity.findViewById(R.id.menu_frame);
    final PaddingInsetsListener insetsListener =
        new PaddingInsetsListener.Builder()
            .setInsetsTypeMask(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout())
            .setExcludeTop()
            .build();
    ViewCompat.setOnApplyWindowInsetsListener(menuFrame, insetsListener);
  }

  @NonNull
  protected View getFrame()
  {
    return mFrame;
  }

  @NonNull
  private View getRoutingOptionsBtnContainer()
  {
    return mRoutingOptionsBanner;
  }

  private void setupRouterButtons()
  {
    setupRouterButton(R.id.vehicle, R.drawable.ic_car, this::onVehicleModeSelected);
    setupRouterButton(R.id.pedestrian, R.drawable.ic_pedestrian, this::onPedestrianModeSelected);
    //    setupRouterButton(R.id.taxi, R.drawable.ic_taxi, this::onTaxiModeSelected);
    setupRouterButton(R.id.transit, app.organicmaps.sdk.R.drawable.ic_route_planning_metro_40px,
                      this::onTransitModeSelected);
    setupRouterButton(R.id.bicycle, R.drawable.ic_bike, this::onBicycleModeSelected);
    setupRouterButton(R.id.ruler, app.organicmaps.sdk.R.drawable.ic_ruler_route, this::onRulerModeSelected);
  }

  private void onTransitModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Transit);
  }

  private void onBicycleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Bicycle);
  }

  private void onRulerModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Ruler);
  }

  private void onPedestrianModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Pedestrian);
  }

  private void onVehicleModeSelected(@NonNull View v)
  {
    RoutingController.get().setRouterType(Router.Vehicle);
  }

  @Override
  public void onUpClick()
  {
    // Ignore the event if the back and start buttons are pressed at the same time.
    // See {@link #RoutingBottomMenuController.setStartButton()}.
    if (RoutingController.get().isNavigating())
      return;
    RoutingController.get().cancel();
  }

  boolean checkFrameHeight()
  {
    if (mFrameHeight > 0)
      return true;

    mFrameHeight = mFrame.getHeight();
    return (mFrameHeight > 0);
  }

  private void updateProgressLabels()
  {
    RoutingController.BuildState buildState = RoutingController.get().getBuildState();

    final boolean ready = (buildState == RoutingController.BuildState.BUILT);

    if (!ready)
    {
      mRoutingBottomMenuController.hideAltitudeChartAndRoutingDetails();
      return;
    }

    if (isTransitType())
    {
      TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
      if (info != null)
        mRoutingBottomMenuController.showTransitInfo(info);
      return;
    }

    if (isRulerType())
    {
      RoutingInfo routingInfo = RoutingController.get().getCachedRoutingInfo();
      if (routingInfo != null)
        mRoutingBottomMenuController.showRulerInfo(Framework.nativeGetRoutePoints(), routingInfo.distToTarget);
      return;
    }

    final boolean showStartButton = !RoutingController.get().isRulerRouterType();
    mRoutingBottomMenuController.setStartButton(showStartButton);
    mRoutingBottomMenuController.showAltitudeChartAndRoutingDetails();
  }

  public void updateBuildProgress(int progress, @NonNull Router router)
  {
    UiUtils.invisible(mProgressVehicle, mProgressPedestrian, mProgressTransit, mProgressBicycle, mProgressRuler);
    WheelProgressView progressView = switch (router)
    {
      case Vehicle ->
      {
        mRouterTypes.check(R.id.vehicle);
        yield mProgressVehicle;
      }
      case Pedestrian ->
      {
        mRouterTypes.check(R.id.pedestrian);
        yield mProgressPedestrian;
      }
      case Transit ->
      {
        mRouterTypes.check(R.id.transit);
        yield mProgressTransit;
      }
      case Bicycle ->
      {
        mRouterTypes.check(R.id.bicycle);
        yield mProgressBicycle;
      }
      case Ruler ->
      {
        mRouterTypes.check(R.id.ruler);
        yield mProgressRuler;
      }
      default -> throw new IllegalArgumentException("unknown router: " + router);
    };

    RoutingToolbarButton button = mRouterTypes.findViewById(mRouterTypes.getCheckedRadioButtonId());
    button.progress();

    updateProgressLabels();

    if (!RoutingController.get().isBuilding())
    {
      button.complete();
      return;
    }

    UiUtils.show(progressView);
    progressView.setPending(progress == 0);
    if (progress != 0)
      progressView.setProgress(progress);
  }

  private boolean isTransitType()
  {
    return RoutingController.get().isTransitType();
  }

  private boolean isRulerType()
  {
    return RoutingController.get().isRulerRouterType();
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    mRoutingBottomMenuController.saveRoutingPanelState(outState);
    outState.putBoolean(BUNDLE_HAS_DRIVING_OPTIONS_VIEW, UiUtils.isVisible(mRoutingOptionsBanner));
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    mRoutingBottomMenuController.restoreRoutingPanelState(state);
    boolean hasView = state.getBoolean(BUNDLE_HAS_DRIVING_OPTIONS_VIEW);
    if (hasView)
      showRoutingOptionsView();
  }

  public void showAddStartFrame()
  {
    mRoutingBottomMenuController.showAddStartFrame();
  }

  public void showAddFinishFrame()
  {
    mRoutingBottomMenuController.showAddFinishFrame();
  }

  private void addRoutingOptionButton(RoadType type, int titleId, LinearLayout container)
  {
    Router rt = RoutingController.get().getLastRouterType();
    if (!RoutingOptions.hasOption(type, rt))
      return;

    MaterialButton btn = (MaterialButton) LayoutInflater.from(container.getContext())
                             .inflate(R.layout.routing_option_button, container, false);
    btn.setText(titleId);
    btn.setOnClickListener(v -> {
      RoutingOptions.removeOption(type, rt);
      //RoutingController.get().rebuildLastRoute();
      container.removeView(v);
      if (!RoutingOptions.hasAnyOptions(rt))
        UiUtils.hide(mRoutingOptionsBanner);

      container.post(() -> {
          RoutingController.get().rebuildLastRoute();
      });
    });
    container.addView(btn);
  }

  public void showRoutingOptionsView()
  {
    mRoutingOptionsBanner.addOnLayoutChangeListener(mRoutingOptionsLayoutListener);
    UiUtils.show(mRoutingOptionsBanner);
    boolean hasAnyOptions = !isRulerType() && RoutingOptions.hasAnyOptions(RoutingController.get().getLastRouterType());
    if (hasAnyOptions)
    {
      LinearLayout container = mRoutingOptionsBanner.findViewById(R.id.routing_options_buttons_container);
      container.removeAllViews();
      addRoutingOptionButton(RoadType.Ferry, R.string.avoid_ferry, container);
      addRoutingOptionButton(RoadType.Motorway, R.string.avoid_motorways, container);
      addRoutingOptionButton(RoadType.Paved, R.string.avoid_paved, container);
      addRoutingOptionButton(RoadType.Dirty, R.string.avoid_unpaved, container);
      addRoutingOptionButton(RoadType.Steps, R.string.avoid_steps, container);
      addRoutingOptionButton(RoadType.Toll, R.string.avoid_tolls, container);
    }
    else
    {
      UiUtils.hide(mRoutingOptionsBanner);
    }
  }

  public void hideRoutingOptionsView()
  {
    UiUtils.hide(mRoutingOptionsBanner);
    mRoutingPlanListener.onRoutingPlanStartAnimate(UiUtils.isVisible(getFrame()));
  }

  public int calcHeight()
  {
    int frameHeight = getFrame().getHeight();
    if (frameHeight == 0)
      return 0;

    View driverOptionsView = getRoutingOptionsBtnContainer();
    int extraOppositeOffset = UiUtils.isVisible(driverOptionsView) ? 0 : driverOptionsView.getHeight();

    return frameHeight - extraOppositeOffset;
  }

  private class SelfTerminatedRoutingOptionsLayoutListener implements View.OnLayoutChangeListener
  {
    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight,
                               int oldBottom)
    {
      mRoutingPlanListener.onRoutingPlanStartAnimate(UiUtils.isVisible(getFrame()));
      mRoutingOptionsBanner.removeOnLayoutChangeListener(this);
    }
  }
}
