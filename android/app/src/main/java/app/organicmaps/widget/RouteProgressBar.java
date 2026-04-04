package app.organicmaps.widget;

import android.content.Context;
import android.content.res.Resources;
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
import app.organicmaps.util.Utils;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.progressindicator.LinearProgressIndicator;

public class RouteProgressBar extends ConstraintLayout
{
  private RouteProgressIndicator mProgressIndicator;
  private ShapeableImageView mNextIntermediateStop;
  private ShapeableImageView mFinalDestination;

  private static class RouteProgressIndicator extends LinearProgressIndicator
  {
    private float mHeight = -1;
    private float mOffset = -1;
    private float mWidth = -1;
    private float mDotRadius = -1;

    Paint mBackgroundPaint;
    Paint mForegroungPaint;
    Paint mIntermediateStopsPaint;

    private double mProgress = 0.0;
    private double[] mIntermediateStops;
    private boolean mIsRtlDirection = false;

    public RouteProgressIndicator(@NonNull Context context)
    {
      this(context, null);
    }

    public RouteProgressIndicator(@NonNull Context context, @Nullable AttributeSet attrs)
    {
      this(context, attrs, 0);
      init(context);
    }

    private void init(Context context)
    {
      // Init paint to draw progress bar background.
      mBackgroundPaint = new Paint();
      mBackgroundPaint.setAntiAlias(true);
      mBackgroundPaint.setColor(ContextCompat.getColor(context, R.color.bg_routing_progress));

      // Init paint to draw progress bar foreground and final destination dot.
      mForegroungPaint = new Paint();
      mForegroungPaint.setAntiAlias(true);
      mForegroungPaint.setColor(ContextCompat.getColor(context, R.color.base_accent));

      // Init paint to draw intermediate stop dots.
      mIntermediateStopsPaint = new Paint();
      mIntermediateStopsPaint.setAntiAlias(true);
      mIntermediateStopsPaint.setColor(ContextCompat.getColor(context,
                                                              R.color.base_accent_pressed));

      // Store RTL current configuration.
      mIsRtlDirection = Utils.isRtlLayoutDirection();
    }

    public RouteProgressIndicator(@NonNull Context context, @Nullable AttributeSet attrs,
                                  int defStyleAttr)
    {
      super(context, attrs, defStyleAttr);
    }

    public void update(double progress, double[] intermediateStops)
    {
      mProgress = progress;
      mIntermediateStops = intermediateStops;
      invalidate();
    }

    private void initDimensions()
    {
      mHeight = getHeight();
      mOffset = mHeight / 2;
      mWidth = getWidth() - 2 * mOffset;
      mDotRadius = mHeight / 3;
    }

    private float calculateXPos(double progress)
    {
      return (float) Math.round(mOffset +
                                mWidth * (mIsRtlDirection? 100.0 - progress : progress) / 100.0);
    }

    @Override
    synchronized protected void onDraw(@NonNull Canvas canvas)
    {
      // Do not call super onDraw(). Full progress bar drawing will be done here.
      // super.onDraw(canvas);

      float progressXPos = calculateXPos(mProgress);
      float finalDestinationDotXPos = calculateXPos(100.0);

      // Draw progress bar background.
      canvas.drawRect(progressXPos, 0, finalDestinationDotXPos, mHeight, mBackgroundPaint);
      canvas.drawCircle(finalDestinationDotXPos, mOffset, mOffset, mBackgroundPaint);

      // Draw progress bar foreground.
      canvas.drawRect(mOffset, 0, progressXPos, mHeight, mForegroungPaint);
      canvas.drawCircle(mOffset, mOffset, mOffset, mForegroungPaint);
      canvas.drawCircle(progressXPos, mOffset, mOffset, mForegroungPaint);

      if (mIntermediateStops == null)
        return;

      if (mOffset <= 0)
        initDimensions();

      // Draw intermediate stops.
      for (double intermediateStopProgress : mIntermediateStops)
        canvas.drawCircle(calculateXPos(intermediateStopProgress), mOffset, mDotRadius,
                          mIntermediateStopsPaint);

      // Draw final destination dot.
      canvas.drawCircle(finalDestinationDotXPos, mOffset, mDotRadius, mForegroungPaint);
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
    init(context);
  }

  private void init(Context context)
  {
    View containerView = inflate(context, R.layout.route_progress_bar, this);
    mProgressIndicator = containerView.findViewById(R.id.progress_indicator);
    mNextIntermediateStop = containerView.findViewById(R.id.next_intermediate_stop);
    mFinalDestination = containerView.findViewById(R.id.final_destination);
  }

  public void update(double completionPercent, int indexOfNextStop,
                     boolean showInfoToFinalDestination, double[] intermediateStopsProgress)
  {
    // Update indicator progress and intermediate stops.
    mProgressIndicator.update(completionPercent, intermediateStopsProgress);

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

      if ((intermediateStopsProgress == null) ||
          (indexOfNextStop > intermediateStopsProgress.length))
      {
        // Hide next intermediate stop icon.
        UiUtils.hide(mNextIntermediateStop);
        return;
      }

      // Set next intermediate stop icon.
      int nextStopIconId;
      try
      {
        TypedArray iconArray = getResources().obtainTypedArray(R.array.route_stop_icons);
        nextStopIconId = iconArray.getResourceId(indexOfNextStop - 1,
                                                 R.drawable.route_point_20);
        iconArray.recycle();
      }
      catch (Resources.NotFoundException e)
      {
        nextStopIconId = R.drawable.route_point_20;
      }
      mNextIntermediateStop.setImageDrawable(AppCompatResources.getDrawable(getContext(),
                                                                            nextStopIconId));

      // Move next intermediate stop icon.
      int startPos = (int) mProgressIndicator.calculateXPos(
        intermediateStopsProgress[indexOfNextStop - 1]) - mNextIntermediateStop.getWidth() / 2;
      ConstraintLayout constraintLayout = findViewById(R.id.route_progress_bar_layout);
      ConstraintSet constraintSet = new ConstraintSet();
      constraintSet.clone(constraintLayout);
      constraintSet.setMargin(R.id.next_intermediate_stop, ConstraintSet.START, startPos);
      constraintSet.applyTo(constraintLayout);

      // Show next intermediate stop icon.
      UiUtils.show(mNextIntermediateStop);
    }
  }
}
