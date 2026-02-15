package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;

import androidx.annotation.Nullable;

import app.organicmaps.R;

public class SpeedLimitView extends BaseSignView
{
  private int    mSpeedLimit = -1;
  private boolean mAlert     = false;
  private String mSpeedStr   = "-1";
  private final int unlimitedBorderColor;
  private final int unlimitedStripeColor;

  public SpeedLimitView(Context ctx, @Nullable AttributeSet attrs)
  {
    super(ctx, attrs);

    setBorderWidthRatio(0.2f);
    setBorderInsetRatio(0.05f);

    try (TypedArray styleAttrs = ctx.getTheme().obtainStyledAttributes(attrs, R.styleable.SpeedLimitView, 0, 0))
    {
      final int bgColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitBackgroundColor, DefaultValues.BACKGROUND_COLOR);
      final int borderColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitBorderColor, DefaultValues.BORDER_COLOR);
      final int alertColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitAlertColor, DefaultValues.ALERT_COLOR);
      final int textColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitTextColor, DefaultValues.TEXT_COLOR);
      final int txtAlertColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitTextAlertColor, DefaultValues.TEXT_ALERT_COLOR);
      setColors(bgColor, borderColor, alertColor, textColor, txtAlertColor);

      unlimitedBorderColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitUnlimitedBorderColor, DefaultValues.UNLIMITED_BORDER_COLOR);
      unlimitedStripeColor = styleAttrs.getColor(R.styleable.SpeedLimitView_speedLimitUnlimitedStripeColor, DefaultValues.UNLIMITED_STRIPE_COLOR);

      if (isInEditMode())
      {
        mSpeedLimit = styleAttrs.getInt(R.styleable.SpeedLimitView_speedLimitEditModeSpeedLimit, 60);
        mAlert = styleAttrs.getBoolean(R.styleable.SpeedLimitView_speedLimitEditModeAlert, false);
        mSpeedStr = Integer.toString(mSpeedLimit);
      }
    }
  }

  public void setSpeedLimit(int limit, boolean alert)
  {
    if (mSpeedLimit != limit)
    {
      mSpeedLimit = limit;
      mSpeedStr   = Integer.toString(limit);
      requestLayout();
    }
    mAlert = alert;
    configureTextSize();
    invalidate();
  }

  @Nullable
  @Override
  protected String getValueString()
  {
    return (mSpeedLimit > 0 ? mSpeedStr : null);
  }

  @Override
  protected boolean isAlert()
  {
    return mAlert;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    final float cx = mWidth/2f, cy = mHeight/2f;

    if (mSpeedLimit == 0) // 0 means unlimited speed (maxspeed=none)
    {
      // background
      mBackgroundPaint.setColor(mBackgroundColor);
      canvas.drawCircle(cx, cy, mRadius, mBackgroundPaint);

      // black border
      mBorderPaint.setColor(unlimitedBorderColor);
      mBorderPaint.setStrokeWidth(mBorderWidth);
      canvas.drawCircle(cx, cy, mBorderRadius, mBorderPaint);

      // draw 5 diagonal stripes
      drawUnlimitedStripes(canvas, cx, cy);
    }
    else
    {
      // delegate to BaseSignView’s onDraw
      super.onDraw(canvas);
    }
  }

  private void drawUnlimitedStripes(Canvas c, float cx, float cy)
  {
    final Paint stripe = new Paint(Paint.ANTI_ALIAS_FLAG);
    stripe.setColor(unlimitedStripeColor);
    stripe.setStrokeWidth(mBorderWidth * 0.4f);

    final float radius = mRadius * 0.8f; // Shorten to 80% of full radius
    final float diag = (float) (1/Math.sqrt(2)); // 45 degrees
    final float dx = -diag, dy = +diag;
    final float px = -dy, py = +dx; // Perpendicular
    final float step = radius * 0.15f; // Spacing

    for (int i = -2; i <= 2; i++)
    {
      final float ox = px * step * i;
      final float oy = py * step * i;
      final float sx = cx + dx * radius + ox;
      final float sy = cy + dy * radius + oy;
      final float ex = cx - dx * radius + ox;
      final float ey = cy - dy * radius + oy;
      c.drawLine(sx, sy, ex, ey, stripe);
    }
  }


  private interface DefaultValues
  {
    int BACKGROUND_COLOR       = 0xFFFFFFFF;
    int BORDER_COLOR           = 0xFFFF0000;
    int ALERT_COLOR            = 0xFFFF0000;
    int TEXT_COLOR             = 0xFF000000;
    int TEXT_ALERT_COLOR       = 0xFFFFFFFF;
    int UNLIMITED_BORDER_COLOR = 0xFF000000;
    int UNLIMITED_STRIPE_COLOR = 0xFF000000;
  }
}
