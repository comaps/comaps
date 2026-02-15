package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Pair;

import androidx.annotation.Nullable;

import app.organicmaps.R;
import app.organicmaps.sdk.util.StringUtils;

public class CurrentSpeedView extends BaseSignView
{
  private double mSpeedMps = -1.0;
  private String mSpeedStr = "--";

  public CurrentSpeedView(Context ctx, @Nullable AttributeSet attrs)
  {
    super(ctx, attrs);

    setBorderWidthRatio(0.1f);
    setBorderInsetRatio(0.05f);

    try (TypedArray a = ctx.getTheme()
            .obtainStyledAttributes(attrs, R.styleable.CurrentSpeedView /* reuse same attrs or define new */ , 0, 0))
    {
      int bg   = a.getColor(R.styleable.CurrentSpeedView_currentSpeedBackgroundColor, DefaultValues.BACKGROUND_COLOR);
      int bd   = a.getColor(R.styleable.CurrentSpeedView_currentSpeedBorderColor,     DefaultValues.BORDER_COLOR);
      int tc   = a.getColor(R.styleable.CurrentSpeedView_currentSpeedTextColor,       DefaultValues.TEXT_COLOR);
      setColors(bg, bd, 0, tc, 0);

      if (isInEditMode())
      {
        mSpeedMps = a.getInt(R.styleable.CurrentSpeedView_currentSpeedEditModeCurrentSpeed,  50);
        mSpeedStr = Integer.toString((int)mSpeedMps);
      }
    }
  }

  public void setCurrentSpeed(double mps)
  {
    mSpeedMps = mps;
    if (mps < 0)
    {
      mSpeedStr = "--";
    }
    else
    {
      Pair<String,String> su = StringUtils.nativeFormatSpeedAndUnits(mps);
      mSpeedStr = su.first;
    }
    requestLayout();
    configureTextSize();
    invalidate();
  }

  @Nullable
  @Override
  protected String getValueString()
  {
    return mSpeedStr;
  }

  @Override
  protected boolean isAlert()
  {
    return false;
  }

  private interface DefaultValues
  {
    int BACKGROUND_COLOR    = 0xFFFFFFFF;
    int BORDER_COLOR        = 0xFF000000;
    int TEXT_COLOR          = 0xFF000000;
  }
}
