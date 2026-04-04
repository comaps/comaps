package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.constraintlayout.widget.ConstraintSet;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.util.UiUtils;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.progressindicator.LinearProgressIndicator;

public class RouteProgressBar extends ConstraintLayout
{
  private View mContainerView;
  private RouteProgressIndicator mProgressIndicator;
  private ShapeableImageView mNextIntermediateStop;
  private ShapeableImageView mFinalDestination;

  private double mGain = -1.0;
  private double mOffset = -1.0;

  private static class RouteProgressIndicator extends LinearProgressIndicator
  {
    Paint mFinalDestinationPaint;
    Paint mFutureStopsPaint;
    Paint mPassedStopsPaint;

    private int mNextStopIndex;
    private double[] mIntermediateStops;

    public RouteProgressIndicator(@NonNull Context context)
    {
      this(context, null);
    }

    public RouteProgressIndicator(@NonNull Context context, @Nullable AttributeSet attrs)
    {
      this(context, attrs, 0);
      init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs)
    {
      mFinalDestinationPaint = new Paint();
      mFinalDestinationPaint.setAntiAlias(true);
      mFinalDestinationPaint.setColor(ContextCompat.getColor(context, R.color.base_accent));

      mFutureStopsPaint = new Paint();
      mFutureStopsPaint.setAntiAlias(true);
      mFutureStopsPaint.setColor(ContextCompat.getColor(context, R.color.base_accent_pressed));

      mPassedStopsPaint = new Paint();
      mPassedStopsPaint.setAntiAlias(true);
      //mPassedStopsPaint.setColor(ContextCompat.getColor(context, R.color.bg_routing_progress));
      mPassedStopsPaint.setColor(ContextCompat.getColor(context, R.color.base_accent_pressed));
    }

    public RouteProgressIndicator(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
    {
      super(context, attrs, defStyleAttr);
    }

    public void updateIntermediateStops(int nextStopIndex, double[] intermediateStops)
    {
      mNextStopIndex = nextStopIndex;
      mIntermediateStops = intermediateStops;
      invalidate();
    }

    @Override
    synchronized protected void onDraw(@NonNull Canvas canvas)
    {
      super.onDraw(canvas);

      if (mIntermediateStops == null)
        return;

      float height = getHeight();
      float offset = height / 2;
      float width = getWidth() - 2 * offset;
      float radius = height / 3;

      // Draw intermediate stops.
      for (int stop = 0; stop < mIntermediateStops.length; stop++)
      {
        float posX = (float) Math.round(offset + width * mIntermediateStops[stop] / 100.0);

        boolean futureStop = (mNextStopIndex > 0) && (stop >= mNextStopIndex - 1);

        canvas.drawCircle(posX, height / 2, radius, futureStop?
                                                    mFutureStopsPaint : mPassedStopsPaint);
      }

      // Draw final destination dot.
      float posX = (float) Math.round(offset + width);
      canvas.drawCircle(posX, height / 2, radius, mFinalDestinationPaint);
    }
  }

  public RouteProgressBar(@NonNull Context context)
  {
    this(context, null);
  }

  public RouteProgressBar(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public RouteProgressBar(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(context, attrs);
  }

  private void init(Context context, AttributeSet attrs)
  {
    mContainerView = inflate(context, R.layout.route_progress_bar, this);
    mProgressIndicator = mContainerView.findViewById(R.id.progress_bar);
    mProgressIndicator.setTrackStopIndicatorSize(0);
    mProgressIndicator.setIndicatorTrackGapSize(0);
    mNextIntermediateStop = mContainerView.findViewById(R.id.next_intermediate_stop);
    mFinalDestination = mContainerView.findViewById(R.id.final_destination);
  }

  public void update(double completionPercent, int indexOfNextStop,
                     boolean showInfoToFinalDestination, double[] intermediateStopsProgress)
  {
    if (intermediateStopsProgress != null)
    {
      // Update next stops icons in progress indicator.
      mProgressIndicator.updateIntermediateStops(indexOfNextStop, intermediateStopsProgress);
    }

    // Start progress at 1% according to M3 guidelines.
    final int progress = (completionPercent < 1) ? 1 : (int) completionPercent;
    mProgressIndicator.setProgressCompat(progress, true);

    if ((mGain < 0.0) && (!calculateGainAndOffset()))
    {
      UiUtils.hide(mNextIntermediateStop);
      return;
    }

    if ((showInfoToFinalDestination) || (indexOfNextStop <= 0))
    {
      // Show final destination icon.
      UiUtils.show(mFinalDestination);

      // Hide next intermediate stop icon.
      UiUtils.hide(mNextIntermediateStop);
    }
    else
    {
      // Hide final destination icon.
      UiUtils.hide(mFinalDestination);

      // Set next intermediate stop icon.
      TypedArray iconArray = getResources().obtainTypedArray(R.array.route_stop_icons);
      int nextStopIconId = iconArray.getResourceId(indexOfNextStop - 1,
                                                   R.drawable.route_point_20);
      iconArray.recycle();
      mNextIntermediateStop.setImageDrawable(AppCompatResources.getDrawable(getContext(),
                                                                            nextStopIconId));

      // Move next intermediate stop icon.
      int startPos = (int) Math.round(intermediateStopsProgress[indexOfNextStop - 1] * mGain +
                                      mOffset);
      ConstraintLayout constraintLayout = findViewById(R.id.route_progress_bar_layout);
      ConstraintSet constraintSet = new ConstraintSet();
      constraintSet.clone(constraintLayout);
      constraintSet.setMargin(R.id.next_intermediate_stop, ConstraintSet.START, startPos);
      constraintSet.applyTo(constraintLayout);

      // Show next intermediate stop icon.
      UiUtils.show(mNextIntermediateStop);
    }
  }

  private boolean calculateGainAndOffset()
  {
    double min = mProgressIndicator.getLeft();
    double max = mProgressIndicator.getRight();

    // Return false if layout has not been created yet.
    if (max <= 0)
      return false;

    int iconSize = mContainerView.findViewById(R.id.next_intermediate_stop).getWidth();

    mGain = (max - min) / 100.0;
    mOffset = min - iconSize / 2.0;

    return true;
  }
}
