package app.organicmaps.widget.placepage;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.textview.MaterialTextView;

import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.Review;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.widget.StarRatingView;

import java.time.format.DateTimeFormatter;
import java.time.format.FormatStyle;
import java.util.List;
import java.util.Locale;

public final class ReviewListAdapter extends RecyclerView.Adapter<ReviewListAdapter.ReviewDataHolder>
{
  private static final String TAG = ReviewListAdapter.class.getSimpleName();

  @NonNull
  private final List<Review> mReviews;
  @NonNull
  private final DateTimeFormatter mDateFormatter;

  /**
   * @param reviews the reviews to display
   * @param locale  the locale for displaying the dates
   */
  ReviewListAdapter(@NonNull List<Review> reviews, @NonNull Locale locale)
  {
    mReviews = reviews;
    mDateFormatter = DateTimeFormatter.ofLocalizedDate(FormatStyle.MEDIUM).withLocale(locale);
  }

  @NonNull
  @Override
  public ReviewDataHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    return new ReviewDataHolder(inflater.inflate(R.layout.item_review, parent, false), mDateFormatter);
  }

  @Override
  public void onBindViewHolder(@NonNull ReviewDataHolder holder, int position)
  {
    if (position < 0 || position >= mReviews.size())
    {
      Logger.w(TAG, String.format(Locale.ROOT, "invalid review index %d, reviews list size: %d", position, mReviews.size()));
      return;
    }
    holder.showReview(mReviews.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mReviews.size();
  }

  public static final class ReviewDataHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final StarRatingView mStarRating;
    @NonNull
    private final MaterialTextView mDate;
    @NonNull
    private final MaterialTextView mAuthor;
    @NonNull
    private final MaterialTextView mOpinion;
    @NonNull
    private final DateTimeFormatter mDateFormatter;

    public ReviewDataHolder(@NonNull View itemView, @NonNull DateTimeFormatter dateFormatter)
    {
      super(itemView);
      mStarRating = itemView.findViewById(R.id.star_rating);
      mDate = itemView.findViewById(R.id.date);
      mAuthor = itemView.findViewById(R.id.author);
      mOpinion = itemView.findViewById(R.id.opinion);
      mDateFormatter = dateFormatter;
    }

    public void showReview(@NonNull Review review)
    {
      mStarRating.setRating(review.starRating());
      mDate.setText(review.date().format(mDateFormatter));
      mAuthor.setText(review.author());
      mOpinion.setText(review.opinion());
    }
  }
}
