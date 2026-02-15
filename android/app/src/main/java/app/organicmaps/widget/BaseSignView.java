package app.organicmaps.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public abstract class BaseSignView extends View
{
    private float mBorderWidthRatio = 0.1f;
    protected void setBorderWidthRatio(float ratio) {
        mBorderWidthRatio = ratio;
    }

    private float mBorderInsetRatio = 0f;
    protected void setBorderInsetRatio(float ratio) {
        mBorderInsetRatio = ratio;
    }

    // colors
    protected int mBackgroundColor;
    protected int mBorderColor;
    protected int mAlertColor;
    protected int mTextColor;
    protected int mTextAlertColor;

    // paints
    protected final Paint mBackgroundPaint;
    protected final Paint mBorderPaint;
    protected final Paint mTextPaint;

    // geometry
    protected float mWidth;
    protected float mHeight;
    protected float mRadius;
    protected float mBorderWidth;
    protected float mBorderRadius;

    public BaseSignView(Context ctx, @Nullable AttributeSet attrs)
    {
        super(ctx, attrs);
        mBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBorderPaint.setStyle(Paint.Style.STROKE);
        mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setTextAlign(Paint.Align.CENTER);
        mTextPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
    }

    protected void setColors(int backgroundColor,
                             int borderColor,
                             int alertColor,
                             int textColor,
                             int textAlertColor)
    {
        mBackgroundColor   = backgroundColor;
        mBorderColor       = borderColor;
        mAlertColor        = alertColor;
        mTextColor         = textColor;
        mTextAlertColor    = textAlertColor;

        mBackgroundPaint.setColor(mBackgroundColor);
        mBorderPaint.setColor(mBorderColor);
        mTextPaint.setColor(mTextColor);
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        final float paddingX = getPaddingLeft() + getPaddingRight();
        final float paddingY = getPaddingTop()  + getPaddingBottom();
        mWidth  = width - paddingX;
        mHeight = height - paddingY;
        mRadius = Math.min(mWidth, mHeight) / 2f;
        mBorderWidth  = mRadius * mBorderWidthRatio;
        // subtract half the stroke PLUS the extra inset
        final float gap = mRadius * mBorderInsetRatio;
        mBorderRadius = mRadius - (mBorderWidth / 2f) - gap;
        configureTextSize();
    }

    @Override
    protected void onDraw(@NonNull Canvas canvas)
    {
        super.onDraw(canvas);
        final String str = getValueString();
        if (str == null) return;

        final float cx = mWidth / 2f;
        final float cy = mHeight / 2f;

        // background & border
        boolean alert = isAlert();
        mBackgroundPaint.setColor(alert ? mAlertColor : mBackgroundColor);
        canvas.drawCircle(cx, cy, mRadius, mBackgroundPaint);
        if (!alert)
        {
            mBorderPaint.setStrokeWidth(mBorderWidth);
            mBorderPaint.setColor(mBorderColor);
            canvas.drawCircle(cx, cy, mBorderRadius, mBorderPaint);
        }

        // text
        mTextPaint.setColor(alert ? mTextAlertColor : mTextColor);
        drawValueString(canvas, cx, cy, str);
    }

    @Override
    public boolean onTouchEvent(@NonNull MotionEvent e)
    {
        final float cx = mWidth / 2f, cy = mHeight / 2f;
        final float dx = e.getX() - cx, dy = e.getY() - cy;
        if ((dx * dx) + (dy * dy) <= (mRadius * mRadius))
        {
            performClick();
            return true;
        }
        return false;
    }

    @Override
    public boolean performClick()
    {
        super.performClick();
        return false;
    }

    private void drawValueString(Canvas c, float cx, float cy, String str)
    {
        Rect b = new Rect();
        mTextPaint.getTextBounds(str, 0, str.length(), b);
        final float y = cy - b.exactCenterY();
        c.drawText(str, cx, y, mTextPaint);
    }

    void configureTextSize()
    {
        String text = getValueString();
        if (text == null) return;
        final float textRadius = mBorderRadius - mBorderWidth;
        final float maxTextSize = 2f * textRadius;
        final float maxTextSize2 = maxTextSize * maxTextSize;
        float lo = 0f, hi = maxTextSize, sz = maxTextSize;
        Rect b = new Rect();
        while (lo <= hi)
        {
            sz = (lo + hi) / 2f;
            mTextPaint.setTextSize(sz);
            mTextPaint.getTextBounds(text, 0, text.length(), b);
            float area = b.width()*b.width() + b.height()*b.height();
            if (area <= maxTextSize2)
                lo = sz + 1f;
            else
                hi = sz - 1f;
        }
        mTextPaint.setTextSize(Math.max(1f, sz));
    }

    /** child must return the string to draw, or null if nothing */
    @Nullable
    protected abstract String getValueString();

    /** child decides if this is in “alert” state */
    protected abstract boolean isAlert();
}
