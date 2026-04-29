package app.organicmaps.downloader;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.core.graphics.Insets;
import androidx.core.view.OnApplyWindowInsetsListener;
import androidx.core.view.WindowInsetsCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils;

final class DownloaderInsetsListener implements OnApplyWindowInsetsListener
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final View mToolbar;
  @NonNull
  private final View mFab;
  @NonNull
  private final View mButton;
  @NonNull
  private final View mCheckUpdatesButton;
  @NonNull
  private final RecyclerView mRecyclerView;

  DownloaderInsetsListener(@NonNull View fragmentView)
  {
    mContext = fragmentView.getContext();
    mToolbar = fragmentView.findViewById(R.id.toolbar);
    mFab = fragmentView.findViewById(R.id.fab);
    mButton = fragmentView.findViewById(R.id.action);
    mCheckUpdatesButton = fragmentView.findViewById(R.id.check_updates);
    mRecyclerView = fragmentView.findViewById(R.id.recycler);
    mRecyclerView.setClipToPadding(false);
  }

  @NonNull
  @Override
  public WindowInsetsCompat onApplyWindowInsets(@NonNull View v, @NonNull WindowInsetsCompat insets)
  {
    final Insets safeInsets = insets.getInsets(WindowInsetUtils.TYPE_SAFE_DRAWING);

    mToolbar.setPadding(safeInsets.left, safeInsets.top, safeInsets.right, mToolbar.getPaddingBottom());

    boolean isAnyButtonVisible =
        UiUtils.isVisible(mFab) || UiUtils.isVisible(mButton) || UiUtils.isVisible(mCheckUpdatesButton);
    if (isAnyButtonVisible)
      applyInsetsToButtons(safeInsets);

    applyInsetsToRecyclerView(safeInsets, isAnyButtonVisible);

    // have to consume all insets here, so child views won't handle them again
    return WindowInsetsCompat.CONSUMED;
  }

  private void applyInsetsToButtons(Insets insets)
  {
    int baseMargin = Utils.dimen(mContext, R.dimen.margin_base);

    ViewGroup.MarginLayoutParams fabParams = (ViewGroup.MarginLayoutParams) mFab.getLayoutParams();
    ViewGroup.MarginLayoutParams buttonParams = (ViewGroup.MarginLayoutParams) mButton.getLayoutParams();
    ViewGroup.MarginLayoutParams checkUpdatesButtonParams =
        (ViewGroup.MarginLayoutParams) mCheckUpdatesButton.getLayoutParams();

    final boolean isButtonVisible = UiUtils.isVisible(mButton);
    final boolean isCheckUpdatesButtonVisible = UiUtils.isVisible(mCheckUpdatesButton);

    checkUpdatesButtonParams.leftMargin = insets.left + baseMargin;
    checkUpdatesButtonParams.rightMargin = insets.right + baseMargin;
    checkUpdatesButtonParams.bottomMargin = insets.bottom;

    buttonParams.leftMargin = insets.left + baseMargin;
    buttonParams.rightMargin = insets.right + baseMargin;
    if (isCheckUpdatesButtonVisible)
      buttonParams.bottomMargin = 0;
    else
      buttonParams.bottomMargin = insets.bottom;

    fabParams.rightMargin = insets.right + baseMargin;
    if (isCheckUpdatesButtonVisible || isButtonVisible)
      fabParams.bottomMargin = baseMargin;
    else
      fabParams.bottomMargin = insets.bottom + baseMargin;

    mFab.requestLayout();
    mButton.requestLayout();
    mCheckUpdatesButton.requestLayout();
  }

  private void applyInsetsToRecyclerView(Insets insets, boolean isAnyButtonVisible)
  {
    int bottomInset = isAnyButtonVisible ? 0 : insets.bottom;

    mRecyclerView.setPadding(mRecyclerView.getPaddingLeft(), mRecyclerView.getPaddingTop(),
                             mRecyclerView.getPaddingRight(), bottomInset);

    final ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) mRecyclerView.getLayoutParams();
    layoutParams.rightMargin = insets.right;
    layoutParams.leftMargin = insets.left;

    mRecyclerView.requestLayout();
  }
}
