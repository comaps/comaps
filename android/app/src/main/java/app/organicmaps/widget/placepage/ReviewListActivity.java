package app.organicmaps.widget.placepage;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

import app.organicmaps.R;
import app.organicmaps.base.BaseToolbarActivity;
import app.organicmaps.sdk.bookmarks.data.Review;

import java.util.ArrayList;

public final class ReviewListActivity extends BaseToolbarActivity
{
  private static final String EXTRA_TITLE = "title";

  @Override
  protected int getToolbarTitle()
  {
    return R.string.place_reviews_title;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ReviewListFragment.class;
  }

  public static void start(@NonNull Context context, @NonNull String title, @NonNull ArrayList<Review> reviews)
  {
    Intent intent = new Intent(context, ReviewListActivity.class)
        .putParcelableArrayListExtra(ReviewListFragment.EXTRA_REVIEWS, reviews)
        .putExtra(EXTRA_TITLE, context.getString(R.string.place_reviews, title));
    context.startActivity(intent);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    String toolbarTitle = getIntent().getStringExtra(EXTRA_TITLE);
    this.getToolbar().setTitle(toolbarTitle);
  }

}
