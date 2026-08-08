package app.organicmaps.bookmarks;

import android.app.Activity;
import android.view.View;
import androidx.annotation.NonNull;
import java.util.function.Consumer;
import app.organicmaps.widget.SearchToolbarController;

public class BookmarksToolbarController extends SearchToolbarController
{
  @NonNull
  private final Runnable mOnDeactivate;
  @NonNull
  private final Consumer<String> mOnSearch;
  @NonNull
  private final Runnable mOnCancel;

  BookmarksToolbarController(@NonNull View root, @NonNull Activity activity,
                              @NonNull Runnable onDeactivate,
                              @NonNull Consumer<String> onSearch,
                              @NonNull Runnable onCancel)
  {
    super(root, activity);
    mOnDeactivate = onDeactivate;
    mOnSearch = onSearch;
    mOnCancel = onCancel;
  }

  @Override
  protected boolean alwaysShowClearButton()
  {
    return true;
  }

  @Override
  protected void onClearClick()
  {
    super.onClearClick();
    mOnDeactivate.run();
  }

  @Override
  protected void onTextChanged(String query)
  {
    if (hasQuery())
      mOnSearch.accept(getQuery());
    else
      mOnCancel.run();
  }

  @Override
  protected boolean showBackButton()
  {
    return false;
  }
}
