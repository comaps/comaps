package app.organicmaps.car.screens.base;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import app.organicmaps.car.renderer.Renderer;
import android.location.Location;
import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.util.StringUtils;

public abstract class BaseMapScreen extends BaseScreen
{
  @NonNull
  private final Renderer mSurfaceRenderer;

  @NonNull
  private final LocationListener mCurrentSpeedListener = this::onLocation;

  private static final float LIMIT_LOOKUP_MIN_DISTANCE_M = 15f;
  private static final int LIMIT_CONFIRMATIONS = 2;

  @Nullable
  private Location mLastLookupLocation;
  private double mShownLimitMps = -1;
  private double mPendingLimitMps = -1;
  private int mPendingCount;

  public BaseMapScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer)
  {
    super(carContext);
    mSurfaceRenderer = surfaceRenderer;
  }

  @NonNull
  protected Renderer getSurfaceRenderer()
  {
    return mSurfaceRenderer;
  }

  protected boolean shouldShowRoadSpeedLimit()
  {
    return true;
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    super.onStart(owner);
    if (shouldShowRoadSpeedLimit())
    {
      mShownLimitMps = -1;
      mPendingLimitMps = -1;
      mPendingCount = 0;
      mLastLookupLocation = null;
      getSurfaceRenderer().hideSpeedLimit();
    }
    MwmApplication.from(getCarContext()).getLocationHelper().addListener(mCurrentSpeedListener);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    MwmApplication.from(getCarContext()).getLocationHelper().removeListener(mCurrentSpeedListener);
    Framework.nativeResetRoadSpeedLimit();
    super.onStop(owner);
  }

  private static final double MIN_VISIBLE_SPEED_MPS = 2.0; // ~7 km/h
  private void updateCurrentSpeed(@Nullable Location location)
  {
    if (location != null && location.hasSpeed() && location.getSpeed() >= MIN_VISIBLE_SPEED_MPS)
      getSurfaceRenderer().setCurrentSpeed(location.getSpeed());
    else
      getSurfaceRenderer().hideCurrentSpeed();
  }

  private void onLocation(@Nullable Location location)
  {
    updateCurrentSpeed(location);
    if (shouldShowRoadSpeedLimit())
      updateRoadSpeedLimit(location);
  }

  private void updateRoadSpeedLimit(@Nullable Location location)
  {
    if (location != null && location.hasBearing() && location.hasSpeed()
        && location.getSpeed() >= MIN_VISIBLE_SPEED_MPS
        && (mLastLookupLocation == null || location.distanceTo(mLastLookupLocation) >= LIMIT_LOOKUP_MIN_DISTANCE_M))
    {
      mLastLookupLocation = location;
      final double limit = Framework.nativeGetRoadSpeedLimit(location.getLatitude(), location.getLongitude(),
                                                             location.getBearing());
      if (limit == mShownLimitMps)
        mPendingCount = 0;
      else if (limit != mPendingLimitMps)
      {
        mPendingLimitMps = limit;
        mPendingCount = 1;
      }
      else if (++mPendingCount >= LIMIT_CONFIRMATIONS)
        mShownLimitMps = limit;
    }

    // Runs on every fix so the over-limit alert tracks current speed.
    if (mShownLimitMps >= 0)
    {
      final boolean exceeded = mShownLimitMps > 0 && location != null && location.hasSpeed()
                               && location.getSpeed() > mShownLimitMps;
      getSurfaceRenderer().setSpeedLimit(StringUtils.nativeFormatSpeed(mShownLimitMps), exceeded);
    }
    else
      getSurfaceRenderer().hideSpeedLimit();
  }
  }
