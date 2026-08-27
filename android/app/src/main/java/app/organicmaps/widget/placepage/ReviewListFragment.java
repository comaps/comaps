package app.organicmaps.widget.placepage;

import android.os.Build;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.CallSuper;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.recyclerview.widget.DividerItemDecoration;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.sdk.bookmarks.data.Review;

import java.util.List;
import java.util.Objects;

public final class ReviewListFragment extends BaseMwmRecyclerFragment<ReviewListAdapter>
{
  public static final String EXTRA_REVIEWS = "reviews";

  @CallSuper
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    DividerItemDecoration divider = new DividerItemDecoration(requireContext(), DividerItemDecoration.VERTICAL);
    getRecyclerView().addItemDecoration(divider);
    handleBottomNavBar(view);
  }

  @LayoutRes
  protected int getLayoutRes()
  {
    return R.layout.fragment_review_list;
  }

  @NonNull
  @Override
  protected ReviewListAdapter createAdapter()
  {
    List<Review> reviews;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
    {
      reviews = Objects.requireNonNull(requireArguments().getParcelableArrayList(EXTRA_REVIEWS, Review.class));
    }
    else
    {
      //noinspection deprecation
      reviews = Objects.requireNonNull(requireArguments().getParcelableArrayList(EXTRA_REVIEWS));
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
    {
      return new ReviewListAdapter(reviews, getResources().getConfiguration().getLocales().get(0));
    }
    else
    {
      //noinspection deprecation
      return new ReviewListAdapter(reviews, getResources().getConfiguration().locale);
    }
  }

  private void handleBottomNavBar(View view)
  {
    View reviewSource = view.findViewById(R.id.review_source);
    if (reviewSource != null)
    {
      int originalPaddingBottom = reviewSource.getPaddingBottom();
      ViewCompat.setOnApplyWindowInsetsListener(reviewSource, (v, insets) -> {
        Insets navBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
        v.setPadding(
            v.getPaddingLeft(),
            v.getPaddingTop(),
            v.getPaddingRight(),
            originalPaddingBottom + navBars.bottom
        );
        return insets;
      });
    }
  }

}
