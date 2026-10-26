package app.organicmaps.car.screens.base;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import app.organicmaps.car.renderer.Renderer;
import android.location.Location;
import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.location.LocationListener;

public abstract class BaseMapScreen extends BaseScreen
{
  @NonNull
  private final Renderer mSurfaceRenderer;

  @NonNull
  private final LocationListener mCurrentSpeedListener = this::updateCurrentSpeed;

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

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    super.onStart(owner);
    MwmApplication.from(getCarContext()).getLocationHelper().addListener(mCurrentSpeedListener);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    MwmApplication.from(getCarContext()).getLocationHelper().removeListener(mCurrentSpeedListener);
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
}
