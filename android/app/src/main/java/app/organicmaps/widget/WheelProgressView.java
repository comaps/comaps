package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;
import androidx.core.graphics.drawable.DrawableCompat;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.progressindicator.CircularProgressIndicator;

public class WheelProgressView extends FrameLayout
{
  private static final int DEFAULT_THICKNESS = 4;
  private CircularProgressIndicator mProgress;
  private static Drawable defaultDrawable;

  public WheelProgressView(Context context)
  {
    this(context, null);
  }

  public WheelProgressView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public WheelProgressView(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
    init(context, attrs);
    setSaveEnabled(true);
  }

  private void init(Context context, AttributeSet attrs)
  {
    View v = inflate(context, R.layout.wheelprogress_view, this);
    mProgress = v.findViewById(R.id.progress);
    ShapeableImageView mDrawable = v.findViewById(R.id.cancel);
    final TypedArray typedArray = context.obtainStyledAttributes(attrs, R.styleable.WheelProgressView, 0, 0);
    mProgress.setTrackThickness(
        typedArray.getDimensionPixelSize(R.styleable.WheelProgressView_wheelThickness, DEFAULT_THICKNESS));
    mProgress.setIndicatorColor(typedArray.getColor(R.styleable.WheelProgressView_wheelProgressColor, Color.WHITE));
    mProgress.setTrackColor(typedArray.getColor(R.styleable.WheelProgressView_wheelSecondaryColor, Color.GRAY));
    Drawable centerDrawable = typedArray.getDrawable(R.styleable.WheelProgressView_centerDrawable);
    mDrawable.setImageDrawable(centerDrawable != null ? centerDrawable : getDefaultDrawable(context));
    typedArray.recycle();
  }

  @NonNull
  private static Drawable getDefaultDrawable(@NonNull Context context)
  {
    if (defaultDrawable == null) {
      Drawable drawable = ContextCompat.getDrawable(context.getApplicationContext(), R.drawable.ic_close);
      defaultDrawable = DrawableCompat.wrap(drawable).mutate();
    }
    DrawableCompat.setTint(defaultDrawable, ThemeUtils.getColor(context, R.attr.iconTint));
    return defaultDrawable;
  }

  public void setProgress(int progress)
  {
    if (progress > mProgress.getProgress())
      mProgress.setProgressCompat(progress, true);
  }

  public void setPending(boolean pending)
  {
    if (pending && !mProgress.isIndeterminate())
      mProgress.setProgressCompat(0, false);
    mProgress.setIndeterminate(pending);
  }

  public void reset()
  {
    mProgress.setProgressCompat(0, false);
  }
}
