package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;

import app.organicmaps.R;
import app.organicmaps.sdk.util.log.Logger;

import java.util.Locale;
import java.util.Objects;

public final class StarRatingView extends View
{
  private static final String TAG = StarRatingView.class.getSimpleName();

  private static final int MIN_RATING = 1;
  private static final int MAX_RATING = 5;

  /**
   * The point from the top of the star at which the baseline should be placed. Used to
   * visually align the stars with surrounding text. 1.0 places the baseline exactly at the bottom of the star.
   */
  private static final double STAR_BASELINE_ADJUSTMENT = 0.95;

  private float mRating;
  private final int mStarSize;
  @NonNull
  private final Drawable mStarDrawable;
  private final int mFgColor;
  private final int mBgColor;

  public StarRatingView(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    try (TypedArray typedArray = context.obtainStyledAttributes(attrs, R.styleable.StarRatingView))
    {
      mRating = validateRating(typedArray.getFloat(R.styleable.StarRatingView_rating, MIN_RATING + (float) (MAX_RATING - MIN_RATING) / 2));
      mStarSize = (int) typedArray.getDimension(R.styleable.StarRatingView_size, R.dimen.text_size_body_0);
      mStarDrawable = Objects.requireNonNull(ContextCompat.getDrawable(context, R.drawable.ic_star));
      mFgColor = ContextCompat.getColor(context, R.color.fg_rating_star);
      mBgColor = ContextCompat.getColor(context, R.color.bg_rating_star);
    }
  }

  public StarRatingView(Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public void setRating(float rating)
  {
    mRating = validateRating(rating);
    setContentDescription(getContext().getString(R.string.rating_stars_description, mRating));
    invalidate();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    int desiredWidth = mStarSize * MAX_RATING + getPaddingLeft() + getPaddingRight();
    int desiredHeight = mStarSize + getPaddingTop() + getPaddingBottom();
    setMeasuredDimension(resolveSize(desiredWidth, widthMeasureSpec), resolveSize(desiredHeight, heightMeasureSpec));
  }

  @Override
  public int getBaseline()
  {
    return (int) (getPaddingTop() + mStarSize * STAR_BASELINE_ADJUSTMENT);
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    super.onDraw(canvas);

    mStarDrawable.setTint(mBgColor);
    drawStars(canvas);

    mStarDrawable.setTint(mFgColor);
    int fillWidth = (int) (mStarSize * mRating);
    canvas.save();
    if (getLayoutDirection() == View.LAYOUT_DIRECTION_RTL)
    {
      int rightEdge = getWidth() - getPaddingRight();
      canvas.clipRect(rightEdge - fillWidth, 0, rightEdge, getHeight());
    } else {
      canvas.clipRect(getPaddingLeft(), 0, getPaddingLeft() + fillWidth, getHeight());
    }
    drawStars(canvas);
    canvas.restore();
  }

  private void drawStars(@NonNull Canvas canvas)
  {
    for (int i = 0; i < MAX_RATING; i++)
    {
      int left = getPaddingLeft() + i * mStarSize;
      mStarDrawable.setBounds(left, getPaddingTop(), left + mStarSize, getPaddingTop() + mStarSize);
      mStarDrawable.draw(canvas);
    }
  }

  private static float validateRating(float rating)
  {
    if (rating < MIN_RATING || rating > MAX_RATING)
    {
      Logger.w(TAG, String.format(Locale.ROOT, "rating out of bounds: %1$f", rating));
      return MIN_RATING;
    }
    return rating;
  }
}
