package app.organicmaps.widget.menu;

import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.util.Pair;
import android.view.View;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.preference.PreferenceManager;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.RoutingInfo.RoutingSessionState;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.DateUtils;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.UiUtils;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.progressindicator.CircularProgressIndicator;
import com.google.android.material.progressindicator.LinearProgressIndicator;
import com.google.android.material.textview.MaterialTextView;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.TimeUnit;

import static androidx.core.content.ContextCompat.getString;

public class NavMenu
{
  private final BottomSheetBehavior<View> mNavBottomSheetBehavior;
  private final View mBottomSheetBackground;
  private final View mHeaderFrame;

  private final ShapeableImageView mTts;
  private final View mEtaViewContainer;
  private final MaterialTextView mEtaValue;
  private final MaterialTextView mEtaAmPm;
  private final View mTimeValuesContainer;
  private final MaterialTextView mEtaDestination;
  private final ShapeableImageView mEtaIcon;
  private final MaterialTextView mTimeHourValue;
  private final MaterialTextView mTimeHourUnits;
  private final MaterialTextView mTimeMinuteValue;
  private final MaterialTextView mTimeMinuteUnits;
  private final View mDistanceViewContainer;
  private final MaterialTextView mDistanceValue;
  private final MaterialTextView mDistanceUnits;
  private final LinearProgressIndicator mRouteProgress;
  private final MaterialTextView mRoutingState;
  private final CircularProgressIndicator mRebuildingRouteProgressBar;

  private final AppCompatActivity mActivity;
  private final SharedPreferences mSharedPreferences;
  private final NavMenuListener mNavMenuListener;

  private int mCurrentPeekHeight = 0;

  // Copy of the routing info (to improve UI responsiveness while switching
  // display info between destination and intermediate stop).
  private RoutingInfo mRoutingInfo;

  // Variable to switch the display of distance and time info between final destination or
  // the next intermediate stop.
  private boolean mShowInfoToFinalDestination;

  public interface OnMenuSizeChangedListener
  {
    void OnMenuSizeChange();
  }

  private final OnMenuSizeChangedListener mOnMenuSizeChangedListener;

  public NavMenu(AppCompatActivity activity, NavMenuListener navMenuListener,
                 OnMenuSizeChangedListener onMenuSizeChangedListener)
  {
    mActivity = activity;
    mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(activity);
    mNavMenuListener = navMenuListener;
    final View bottomFrame = mActivity.findViewById(R.id.nav_bottom_frame);
    mHeaderFrame = bottomFrame.findViewById(R.id.line_frame);
    mOnMenuSizeChangedListener = onMenuSizeChangedListener;
    mHeaderFrame.setOnClickListener(v -> toggleNavMenu());
    mHeaderFrame.addOnLayoutChangeListener((view, i, i1, i2, i3, i4, i5, i6, i7) -> setPeekHeight());
    mNavBottomSheetBehavior = BottomSheetBehavior.from(mActivity.findViewById(R.id.nav_bottom_sheet));
    mBottomSheetBackground = mActivity.findViewById(R.id.nav_bottom_sheet_background);
    mBottomSheetBackground.setOnClickListener(v -> collapseNavBottomSheet());
    mBottomSheetBackground.setVisibility(View.GONE);
    mBottomSheetBackground.setAlpha(0);
    mNavBottomSheetBehavior.addBottomSheetCallback(new BottomSheetBehavior.BottomSheetCallback() {
      @Override
      public void onStateChanged(@NonNull View bottomSheet, int newState)
      {
        if (newState == BottomSheetBehavior.STATE_COLLAPSED || newState == BottomSheetBehavior.STATE_HIDDEN)
        {
          mBottomSheetBackground.setVisibility(View.GONE);
          mBottomSheetBackground.setAlpha(0);
        }
        else
        {
          mBottomSheetBackground.setVisibility(View.VISIBLE);
        }
      }

      @Override
      public void onSlide(@NonNull View bottomSheet, float slideOffset)
      {
        mBottomSheetBackground.setAlpha(slideOffset);
      }
    });

    // Bottom frame.
    mEtaViewContainer = bottomFrame.findViewById(R.id.eta_view_container);
    mEtaValue = bottomFrame.findViewById(R.id.eta_value);
    mEtaAmPm = bottomFrame.findViewById(R.id.eta_am_pm);
    mEtaDestination = bottomFrame.findViewById(R.id.eta_destination);
    mEtaIcon = bottomFrame.findViewById(R.id.eta_icon);
    mTimeValuesContainer = bottomFrame.findViewById(R.id.time_values_container);
    mTimeHourValue = bottomFrame.findViewById(R.id.time_hour_value);
    mTimeHourUnits = bottomFrame.findViewById(R.id.time_hour_dimen);
    mTimeMinuteValue = bottomFrame.findViewById(R.id.time_minute_value);
    mTimeMinuteUnits = bottomFrame.findViewById(R.id.time_minute_dimen);
    mDistanceViewContainer = bottomFrame.findViewById(R.id.distance_view_container);
    mDistanceValue = bottomFrame.findViewById(R.id.distance_value);
    mDistanceUnits = bottomFrame.findViewById(R.id.distance_dimen);
    mRouteProgress = bottomFrame.findViewById(R.id.navigation_progress);
    mRoutingState = bottomFrame.findViewById(R.id.routing_state);
    mRebuildingRouteProgressBar = bottomFrame.findViewById(R.id.rebuilding_route_progress_bar);

    // Bottom frame buttons.
    ShapeableImageView mSettings = bottomFrame.findViewById(R.id.settings);
    mSettings.setOnClickListener(v -> onSettingsClicked());
    mTts = bottomFrame.findViewById(R.id.tts_volume);
    mTts.setOnClickListener(v -> onTtsClicked());
    MaterialButton stop = bottomFrame.findViewById(R.id.stop);
    stop.setOnClickListener(v -> onStopClicked());

    // Get preferences for the display of distance and time info (to the final
    // destination or to the next intermediate stop).
    mShowInfoToFinalDestination = mSharedPreferences.getBoolean(
      getString(mActivity, R.string.pref_nav_menu_final_destination), true);
  }

  private void onStopClicked()
  {
    mNavMenuListener.onStopClicked();
  }

  private void onSettingsClicked()
  {
    mNavMenuListener.onSettingsClicked();
  }

  private void onTtsClicked()
  {
    if (!TtsPlayer.isReady())
      Toast.makeText(mActivity, R.string.pref_tts_no_system_tts_short, Toast.LENGTH_SHORT).show();
    TtsPlayer.setEnabled(!TtsPlayer.isEnabled());
    refreshTts();
  }

  private void toggleNavMenu()
  {
    if (getBottomSheetState() == BottomSheetBehavior.STATE_EXPANDED)
      collapseNavBottomSheet();
    else
      expandNavBottomSheet();
  }

  public void setPeekHeight()
  {
    int headerHeight = mHeaderFrame.getHeight();
    if (mCurrentPeekHeight != headerHeight)
    {
      mCurrentPeekHeight = headerHeight;
      mNavBottomSheetBehavior.setPeekHeight(mCurrentPeekHeight);
      mOnMenuSizeChangedListener.OnMenuSizeChange();
    }
  }

  public void collapseNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_COLLAPSED);
  }

  public void expandNavBottomSheet()
  {
    mNavBottomSheetBehavior.setState(BottomSheetBehavior.STATE_EXPANDED);
  }

  public int getBottomSheetState()
  {
    return mNavBottomSheetBehavior.getState();
  }

  public void refreshTts()
  {
    mTts.setImageDrawable(TtsPlayer.isEnabled() ? Graphics.tint(mActivity, R.drawable.ic_voice_on,
                                                                com.google.android.material.R.attr.colorSecondary)
                                                : Graphics.tint(mActivity, R.drawable.ic_voice_off));
  }

  private void updateTime(int timeInSeconds)
  {
    updateTimeLeft(timeInSeconds);
    updateTimeEstimate(timeInSeconds);
  }

  private void updateTimeLeft(int timeInSeconds)
  {
    final long hours = TimeUnit.SECONDS.toHours(timeInSeconds);
    final long minutes = TimeUnit.SECONDS.toMinutes(timeInSeconds) % 60;
    mTimeMinuteValue.setText(String.valueOf(minutes));
    String min = mActivity.getResources().getString(R.string.minute);
    mTimeMinuteUnits.setText(min);
    if (hours == 0)
    {
      UiUtils.hide(mTimeHourUnits, mTimeHourValue);
      return;
    }
    UiUtils.setTextAndShow(mTimeHourValue, String.valueOf(hours));
    String hour = mActivity.getResources().getString(R.string.hour);
    UiUtils.setTextAndShow(mTimeHourUnits, hour);
  }

  private void updateTimeEstimate(int timeInSeconds)
  {
    // Calculate ETA from current local time and remaining seconds.
    final LocalTime localTime = LocalTime.now().plusSeconds(timeInSeconds);

    // String to set the format of the ETA value (24h or AM/PM).
    final String etaValueFormat;

    // Text of the AM/PM view.
    final String etaAmPmText;

    if (DateUtils.is24HourFormat(mTimeMinuteValue.getContext()))
    {
      // 24 hours time format.
      etaValueFormat = "HH:mm";
      etaAmPmText = "";
    }
    else
    {
      // AM/PM time format.
      etaValueFormat = "h:mm";
      etaAmPmText = localTime.format(DateTimeFormatter.ofPattern("a"));
    }

    mEtaValue.setText(localTime.format(DateTimeFormatter.ofPattern(etaValueFormat)));
    mEtaAmPm.setText(etaAmPmText);

    mEtaValue.setOnClickListener(view -> {
      if ((mRoutingInfo == null) || (mRoutingInfo.indexOfNextStop <= 0))
      {
        toggleNavMenu();
      }
      else
      {
        mShowInfoToFinalDestination = !mShowInfoToFinalDestination;
        mSharedPreferences.edit().putBoolean(
          getString(mActivity, R.string.pref_nav_menu_final_destination),
          mShowInfoToFinalDestination).apply();
        updateControls();
      }
    });
  }

  private void updateDistance(Distance distToTarget)
  {
    mDistanceValue.setText(distToTarget.mDistanceStr);
    UiUtils.setTextAndShow(mDistanceUnits,
                           distToTarget.getUnitsStr(mActivity.getApplicationContext()));
  }

  private void updateRouteProgress(double completionPercent)
  {
    // Start progress at 1% according to M3 guidelines.
    final int progress = (completionPercent < 1) ? 1 : (int) completionPercent;
    mRouteProgress.setProgressCompat(progress, true);
  }

  private void updateRoutingSessionState(RoutingSessionState routingSessionState)
  {
    // Show and update route state.
    UiUtils.setTextAndShow(mRoutingState, mActivity.getString(
      switch (routingSessionState)
      {
        case NoValidRoute -> R.string.invalid_route;
        case RouteBuilding -> R.string.building_route;
        case RouteNotStarted -> R.string.route_not_started;
        case OnRoute -> R.string.on_route;
        case RouteNeedsRebuild -> R.string.route_needs_rebuild;
        case RouteFinished -> R.string.route_finished;
        case RouteNoFollowing -> R.string.not_following_route;
        case RouteRebuilding -> R.string.route_recalculating;
      }));
  }

  private Pair<Distance, Integer> updateDestination()
  {
    Distance distance;
    int timeInSeconds;

    // Set destination/stop string and icon.
    String destinationText;
    int iconId;

    if ((mShowInfoToFinalDestination) || (mRoutingInfo.indexOfNextStop <= 0))
    {
      destinationText = mActivity.getString(R.string.destination);
      iconId = R.drawable.route_point_finish;
      distance = mRoutingInfo.distToTarget;
      timeInSeconds = mRoutingInfo.totalTimeInSeconds;
    }
    else
    {
      destinationText = mActivity.getString(R.string.stop);

      TypedArray iconArray = mActivity.getResources().obtainTypedArray(R.array.route_stop_icons);
      iconId = iconArray.getResourceId(mRoutingInfo.indexOfNextStop - 1, R.drawable.route_point_20);
      iconArray.recycle();
      distance = mRoutingInfo.distToNextStop;
      timeInSeconds = mRoutingInfo.timeToNextStop;
    }

    mEtaDestination.setText(destinationText);
    mEtaIcon.setImageDrawable(AppCompatResources.getDrawable(mActivity, iconId));

    return new Pair<>(distance, timeInSeconds);
  }

  public void update(@NonNull RoutingInfo info)
  {
    // Save a copy of the routing info.
    mRoutingInfo = info;

    // Update controls.
    updateControls();
  }

  private void updateControls()
  {
    if (mRoutingInfo == null)
      return;

    // Hide/show & update controls based on routing session state.
    if (RoutingSessionState.isNavigable(mRoutingInfo.routingSessionState))
    {
      // Update destination labels (to final destination or to the next intermediate stop).
      Pair<Distance, Integer> displayInfo = updateDestination();

      // Show & update time info.
      UiUtils.show(mTimeValuesContainer);
      UiUtils.show(mEtaViewContainer);
      updateTime(displayInfo.second);

      // Show & update distance info.
      UiUtils.show(mDistanceViewContainer);
      updateDistance(displayInfo.first);

      // Show & update route progress bar.
      UiUtils.show(mRouteProgress);
      updateRouteProgress(mRoutingInfo.completionPercent);

      // Hide rebuilding route circular progress bar.
      UiUtils.hide(mRebuildingRouteProgressBar);

      // Hide routing session state message.
      mRoutingState.setText("");
      UiUtils.invisible(mRoutingState);
    }
    else
    {
      // Hide time info.
      UiUtils.hide(mTimeValuesContainer);
      UiUtils.hide(mEtaViewContainer);

      // Hide distance info.
      UiUtils.hide(mDistanceViewContainer);

      // Hide route progress bar.
      UiUtils.invisible(mRouteProgress);

      // Show rebuilding route circular progress bar.
      UiUtils.show(mRebuildingRouteProgressBar);

      // Update routing session state message.
      updateRoutingSessionState(mRoutingInfo.routingSessionState);
    }
  }

  public interface NavMenuListener
  {
    void onStopClicked();

    void onSettingsClicked();
  }
}
