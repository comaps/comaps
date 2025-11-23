package app.organicmaps.search;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.AttrRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.textview.MaterialTextView;

import app.organicmaps.R;
import app.organicmaps.sdk.search.SearchResult;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;

class SearchAdapter extends RecyclerView.Adapter<SearchAdapter.SearchDataViewHolder>
{
  private static final int SHORT_HORIZON_CLOSE_MIN = 60;
  private static final int SHORT_HORIZON_OPEN_MIN = 15;

  private final SearchFragment mSearchFragment;
  @Nullable
  private SearchResult[] mResults;

  static abstract class SearchDataViewHolder extends RecyclerView.ViewHolder
  {
    SearchDataViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }

    abstract void bind(@NonNull SearchResult result, int position);
  }

  private static abstract class BaseResultViewHolder extends SearchDataViewHolder
  {
    SearchResult mResult;
    // Position within search results
    int mOrder;

    BaseResultViewHolder(@NonNull View view)
    {
      super(view);
      if (view instanceof MaterialTextView)
      {
        int tintAttr = getTintAttr();
        if (tintAttr != 0)
          Graphics.tint((MaterialTextView) view, tintAttr);
      }
      view.setOnClickListener(v -> processClick(mResult, mOrder));
    }

    @Override
    void bind(@NonNull SearchResult result, int order)
    {
      mResult = result;
      mOrder = order;
      final MaterialTextView titleView = getTitleView();

      if (titleView != null)
        titleView.setText(mResult.getFormattedTitle(titleView.getContext()));
    }

    @AttrRes
    int getTintAttr()
    {
      return androidx.appcompat.R.attr.colorAccent;
    }

    abstract MaterialTextView getTitleView();

    abstract void processClick(SearchResult result, int order);
  }

  private class SuggestViewHolder extends BaseResultViewHolder
  {
    SuggestViewHolder(@NonNull View view)
    {
      super(view);
    }

    @Override
    MaterialTextView getTitleView()
    {
      return (MaterialTextView) itemView;
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.setQuery(result.suggestion, result.type == SearchResult.TYPE_PURE_SUGGEST);
    }
  }

  private class ResultViewHolder extends BaseResultViewHolder
  {
    @NonNull
    final View mFrame;
    @NonNull
    final MaterialTextView mName;
    @NonNull
    final MaterialTextView mOpen;
    @NonNull
    final MaterialTextView mDescription;
    @NonNull
    final MaterialTextView mRegion;
    @NonNull
    final MaterialTextView mDistance;

    @Override
    int getTintAttr()
    {
      return 0;
    }

    ResultViewHolder(@NonNull View view)
    {
      super(view);
      mFrame = view;
      mName = view.findViewById(R.id.title);
      mOpen = view.findViewById(R.id.open);
      mDescription = view.findViewById(R.id.description);
      mRegion = view.findViewById(R.id.region);
      mDistance = view.findViewById(R.id.distance);
    }

    @Override
    MaterialTextView getTitleView()
    {
      return mName;
    }

    @Override
    void bind(@NonNull SearchResult result, int order)
    {
      super.bind(result, order);
      setBackground();

      formatOpeningHours(mResult);
      UiUtils.setTextAndHideIfEmpty(mDescription, mResult.description.description);
      mRegion.setText(mResult.getFormattedAddress(mRegion.getContext()));
      UiUtils.setTextAndHideIfEmpty(mDistance, mResult.description.distance.toString(mFrame.getContext()));
    }

    private void formatOpeningHours(SearchResult result)
    {
      final Resources resources = mSearchFragment.getResources();

      if (result.description.openNow != SearchResult.OPEN_NOW_YES && result.description.openNow != SearchResult.OPEN_NOW_NO)
      {
        // Hide if unknown opening hours state
        UiUtils.hide(mOpen);
        return;
      }

      final boolean isOpen = result.description.openNow == SearchResult.OPEN_NOW_YES;
      final int minsToNextState = isOpen ? result.description.minutesUntilClosed : result.description.minutesUntilOpen;

      final boolean shortHorizonClosing = isOpen && minsToNextState >= 0 && minsToNextState <= SHORT_HORIZON_CLOSE_MIN;
      final boolean shortHorizonOpening = !isOpen && minsToNextState >= 0 && minsToNextState <= SHORT_HORIZON_OPEN_MIN;

      if (shortHorizonClosing || shortHorizonOpening)
      {
        final String minsToChangeStr = resources.getQuantityString(
            R.plurals.minutes_short, Math.max(minsToNextState, 1), Math.max(minsToNextState, 1));
        final String nextChangeFormatted = resources.getString(isOpen ? R.string.closes_in : R.string.opens_in, minsToChangeStr);

        UiUtils.setTextAndShow(mOpen, nextChangeFormatted);
        mOpen.setTextColor(ContextCompat.getColor(mSearchFragment.getContext(), R.color.base_yellow));
      }
      else
      {
        UiUtils.setTextAndShow(mOpen, isOpen ? resources.getString(R.string.editor_time_open) : resources.getString(R.string.closed));
        mOpen.setTextColor(ContextCompat.getColor(mSearchFragment.getContext(), isOpen ? R.color.base_green : R.color.base_red));
      }
    }

    private void setBackground()
    {
      final Context context = mSearchFragment.requireActivity();
      final int itemBg = ThemeUtils.getResource(context, R.attr.clickableBackground);
      mFrame.setBackgroundResource(itemBg);
    }

    @Override
    void processClick(SearchResult result, int order)
    {
      mSearchFragment.showSingleResultOnMap(result, order);
    }
  }

  SearchAdapter(SearchFragment fragment)
  {
    mSearchFragment = fragment;
  }

  @NonNull
  @Override
  public SearchDataViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());

    return switch (viewType)
    {
      case SearchResult.TYPE_SUGGEST, SearchResult.TYPE_PURE_SUGGEST ->
        new SuggestViewHolder(inflater.inflate(R.layout.item_search_suggest, parent, false));
      case SearchResult.TYPE_RESULT ->
        new ResultViewHolder(inflater.inflate(R.layout.item_search_result, parent, false));
      default -> throw new IllegalArgumentException("Unhandled view type given");
    };
  }

  @Override
  public void onBindViewHolder(@NonNull SearchDataViewHolder holder, int position)
  {
    holder.bind(mResults[position], position);
  }

  @Override
  public int getItemViewType(int position)
  {
    return mResults[position].type;
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    int res = 0;
    if (mResults == null)
      return res;

    res += mResults.length;
    return res;
  }

  public void clear()
  {
    refreshData(null);
  }

  void refreshData(@Nullable SearchResult[] results)
  {
    mResults = results;
    notifyDataSetChanged();
  }
}
