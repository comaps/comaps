package app.organicmaps.car.renderer;

import android.app.Presentation;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.car.app.CarContext;
import androidx.car.app.SurfaceContainer;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.widget.CurrentSpeedView;
import app.organicmaps.widget.SpeedLimitView;

@RequiresApi(23)
class SurfaceCallback extends SurfaceCallbackBase
{
  private static final String TAG = SurfaceCallback.class.getSimpleName();

  private static final int SIGN_SIZE_DP = 80;
  private static final int CURRENT_SPEED_WIDTH_DP = 80;
  private static final int CURRENT_SPEED_PADDING_DP = 8;
  private static final int PILL_PADDING_DP = 6;
  private static final int PILL_GAP_DP = 6;
  private static final int PILL_MARGIN_DP = 6;

  private static final String VIRTUAL_DISPLAY_NAME = "OM_Android_Auto_Display";

  @NonNull
  private final MapController mMapController;

  @Nullable
  private FrameLayout mPillOverlay;
  @Nullable
  private LinearLayout mSpeedPill;
  @Nullable
  private SpeedLimitView mSpeedLimitView;
  @Nullable
  private CurrentSpeedView mCurrentSpeedView;
  @Nullable
  private LinearLayout.LayoutParams mSpeedLimitParams;
  @Nullable
  private LinearLayout.LayoutParams mCurrentSpeedParams;

  private final int mSignSize;
  private final int mCurrentSpeedWidth;
  private final int mPillPadding;
  private final int mPillGap;
  private final int mPillMargin;
  private final int mCurrentSpeedPadding;

  @Nullable
  private VirtualDisplay mVirtualDisplay;
  @Nullable
  private Presentation mPresentation;

  public SurfaceCallback(@NonNull CarContext carContext, @NonNull MapController mapController)
  {
    super(carContext);
    mMapController = mapController;
    mMapController.getView().getHolder().addCallback(new SurfaceHolder.Callback() {
      @Override
      public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height)
      {
        mMapController.updateMyPositionRoutingOffset(0);
      }
      @Override
      public void surfaceCreated(@NonNull SurfaceHolder holder)
      {
        mMapController.updateMyPositionRoutingOffset(0);
      }
      @Override
      public void surfaceDestroyed(@NonNull SurfaceHolder holder)
      {}
    });

    final android.util.DisplayMetrics metrics = mCarContext.getResources().getDisplayMetrics();
    mSignSize = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, SIGN_SIZE_DP, metrics);
    mCurrentSpeedWidth = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, CURRENT_SPEED_WIDTH_DP, metrics);
    mCurrentSpeedPadding =
        (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, CURRENT_SPEED_PADDING_DP, metrics);
    mPillPadding = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, PILL_PADDING_DP, metrics);
    mPillGap = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, PILL_GAP_DP, metrics);
    mPillMargin = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, PILL_MARGIN_DP, metrics);

    initSpeedPill();
  }

  @Override
  public void onSurfaceAvailable(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface available " + surfaceContainer);

    mVirtualDisplay =
        mCarContext.getSystemService(DisplayManager.class)
            .createVirtualDisplay(VIRTUAL_DISPLAY_NAME, surfaceContainer.getWidth(), surfaceContainer.getHeight(),
                                  surfaceContainer.getDpi(), surfaceContainer.getSurface(), 0);
    mPresentation = new Presentation(mCarContext, mVirtualDisplay.getDisplay());

    mPresentation.setContentView(prepareViewForPresentation(mMapController.getView()));
    mPresentation.show();
  }

  @Override
  public void onVisibleAreaChanged(@NonNull Rect visibleArea)
  {
    super.onVisibleAreaChanged(visibleArea);

    assert mPillOverlay != null : "mPillOverlay must be initialized";
    mPillOverlay.setLayoutParams(getOverlayLayoutParams());
  }

  @Override
  public void onSurfaceDestroyed(@NonNull SurfaceContainer surfaceContainer)
  {
    Logger.d(TAG, "Surface destroyed");
    if (mPresentation != null)
      mPresentation.dismiss();
    if (mVirtualDisplay != null)
      mVirtualDisplay.release();
  }

  @NonNull
  SpeedLimitView getSpeedLimitView()
  {
    assert mSpeedLimitView != null : "mSpeedLimitView must be initialized";
    return mSpeedLimitView;
  }

  @NonNull
  CurrentSpeedView getCurrentSpeedView()
  {
    assert mCurrentSpeedView != null : "mCurrentSpeedView must be initialized";
    return mCurrentSpeedView;
  }

  void updatePillVisibility()
  {
    assert mCurrentSpeedView != null : "mCurrentSpeedView must be initialized";
    assert mSpeedPill != null : "mSpeedPill must be initialized";
    assert mSpeedLimitParams != null : "mSpeedLimitParams must be initialized";
    assert mCurrentSpeedParams != null : "mCurrentSpeedParams must be initialized";
    assert mSpeedLimitView != null;

    final boolean hasSpeedLimit = mSpeedLimitView.getSpeedLimit() >= 0;
    final boolean hasCurrentSpeed = mCurrentSpeedView.getVisibility() == View.VISIBLE;
    final boolean both = hasSpeedLimit && hasCurrentSpeed;

    mSpeedLimitView.setVisibility(hasSpeedLimit ? View.VISIBLE : View.GONE);
    mSpeedLimitParams.leftMargin = 0; // sign is first in the pill; only current speed needs a gap

    mCurrentSpeedParams.leftMargin = both ? mPillGap : 0;
    mCurrentSpeedParams.width = both ? mCurrentSpeedWidth : mSignSize;

    mSpeedPill.setVisibility((hasSpeedLimit || hasCurrentSpeed) ? View.VISIBLE : View.GONE);
    mSpeedPill.requestLayout();

    applyTheme();
  }

  private void applyTheme()
  {
    assert mSpeedPill != null : "mSpeedPill must be initialized";
    assert mCurrentSpeedView != null : "mCurrentSpeedView must be initialized";
    assert mSpeedLimitView != null : "mSpeedLimitView must be initialized";

    final boolean nightMode = ThemeUtils.isNightMode(mCarContext);
    final int pillBackground = ContextCompat.getColor(
        mCarContext, nightMode ? R.color.car_speed_pill_background_night : R.color.car_speed_pill_background_light);
    final int currentSpeedText = ContextCompat.getColor(
        mCarContext, nightMode ? R.color.car_speed_current_text_night : R.color.car_speed_current_text_light);
    final int signBackground = ContextCompat.getColor(
        mCarContext, nightMode ? R.color.car_speed_sign_background_night : R.color.car_speed_sign_background_light);
    final int currentSpeedAlertText =
        ContextCompat.getColor(mCarContext, nightMode ? R.color.car_speed_current_alert_text_night
                                                      : R.color.car_speed_current_alert_text_light);

    final Drawable background = mSpeedPill.getBackground();
    if (background instanceof GradientDrawable)
      ((GradientDrawable) background.mutate()).setColor(pillBackground);

    mCurrentSpeedView.setTextColor(currentSpeedText);
    mCurrentSpeedView.setAlertTextColor(currentSpeedAlertText);
    mSpeedLimitView.setSignBackgroundColor(signBackground);
  }

  void stopPresenting()
  {
    if (mPresentation != null)
      mPresentation.dismiss();
  }

  void startPresenting()
  {
    if (mPresentation != null)
      mPresentation.show();
  }

  @NonNull
  private View prepareViewForPresentation(@NonNull View view)
  {
    final ViewParent parent = view.getParent();
    if (parent instanceof ViewGroup)
      ((ViewGroup) parent).removeView(view);

    final FrameLayout container = new FrameLayout(mCarContext);
    container.addView(
        view, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    initSpeedPill();
    container.addView(mPillOverlay);

    return container;
  }

  private void initSpeedPill()
  {
    final boolean restoreOldState = mSpeedLimitView != null;
    final int oldSpeedLimit = restoreOldState ? mSpeedLimitView.getSpeedLimit() : -1;
    final boolean oldCurrentSpeedVisible =
        mCurrentSpeedView != null && mCurrentSpeedView.getVisibility() == View.VISIBLE;

    mPillOverlay = new FrameLayout(mCarContext);
    mPillOverlay.setLayoutParams(getOverlayLayoutParams());

    mSpeedPill = new LinearLayout(mCarContext);
    mSpeedPill.setOrientation(LinearLayout.HORIZONTAL);
    mSpeedPill.setGravity(Gravity.CENTER_VERTICAL);
    mSpeedPill.setBackgroundResource(R.drawable.bg_car_speed_pill);
    mSpeedPill.setPadding(mPillPadding, mPillPadding, mPillPadding, mPillPadding);
    mSpeedPill.setVisibility(View.GONE);

    mSpeedLimitView = new SpeedLimitView(mCarContext);
    mSpeedLimitView.setVisibility(View.GONE);
    mSpeedLimitParams = new LinearLayout.LayoutParams(mSignSize, mSignSize);
    mSpeedPill.addView(mSpeedLimitView, mSpeedLimitParams);

    mCurrentSpeedView = new CurrentSpeedView(mCarContext, null);
    mCurrentSpeedView.setFlat(true);
    mCurrentSpeedView.setVisibility(View.GONE);
    mCurrentSpeedView.setPadding(mCurrentSpeedPadding, mCurrentSpeedPadding, mCurrentSpeedPadding,
                                 mCurrentSpeedPadding);
    mCurrentSpeedParams = new LinearLayout.LayoutParams(mCurrentSpeedWidth, mSignSize);
    mSpeedPill.addView(mCurrentSpeedView, mCurrentSpeedParams);

    if (restoreOldState)
      mSpeedLimitView.setSpeedLimit(oldSpeedLimit, false);
    if (oldCurrentSpeedVisible)
      mCurrentSpeedView.setVisibility(View.VISIBLE);
    updatePillVisibility();

    final FrameLayout.LayoutParams pillParams =
        new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    pillParams.gravity = Gravity.END | Gravity.BOTTOM;
    pillParams.rightMargin = mPillMargin;
    pillParams.bottomMargin = mPillMargin;
    mPillOverlay.addView(mSpeedPill, pillParams);
  }

  @NonNull
  private ViewGroup.LayoutParams getOverlayLayoutParams()
  {
    final FrameLayout.LayoutParams layoutParams =
        new FrameLayout.LayoutParams(mVisibleArea.right - mVisibleArea.left, // width
                                     mVisibleArea.bottom - mVisibleArea.top // height
        );
    layoutParams.leftMargin = mVisibleArea.left;
    layoutParams.topMargin = mVisibleArea.top;
    layoutParams.gravity = Gravity.NO_GRAVITY;
    return layoutParams;
  }
}
