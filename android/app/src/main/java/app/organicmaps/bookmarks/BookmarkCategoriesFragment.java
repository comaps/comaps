package app.organicmaps.bookmarks;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.dialog.EditTextDialogFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.CategoryDataSource;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.DataChangedListener;
import app.organicmaps.sdk.bookmarks.data.KmlFileType;
import app.organicmaps.sdk.search.BookmarkSearchListener;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.widget.PlaceholderView;
import app.organicmaps.widget.SearchToolbarController;
import app.organicmaps.widget.recycler.DividerItemDecorationWithPadding;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class BookmarkCategoriesFragment extends BaseMwmRecyclerFragment<BookmarkCategoriesAdapter>
    implements BookmarkManager.BookmarksLoadingListener, CategoryListCallback, OnItemClickListener<BookmarkCategory>,
               OnItemMoreClickListener<BookmarkCategory>, OnItemLongClickListener<BookmarkCategory>,
               BookmarkManager.BookmarksSharingListener, BookmarkSearchListener,
               MenuBottomSheetFragment.MenuBottomSheetInterface

{
  private static final String TAG = BookmarkCategoriesFragment.class.getSimpleName();

  private static final int MAX_CATEGORY_NAME_LENGTH = 60;

  public static final String BOOKMARKS_CATEGORIES_MENU_ID = "BOOKMARKS_CATEGORIES_BOTTOM_SHEET";

  private ActivityResultLauncher<SharingUtils.SharingIntent> shareLauncher;

  @Nullable
  private BookmarkCategory mSelectedCategory;
  @Nullable
  private CategoryEditor mCategoryEditor;

  private boolean mSearchMode = false;
  private long mLastQueryTimestamp = 0;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewGroup mSearchContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SearchToolbarController mToolbarController;
  @Nullable
  private BookmarkListAdapter mBookmarkListAdapter;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private DataChangedListener mCategoriesAdapterObserver;

  private final ActivityResultLauncher<Intent> startBookmarkListForResult =
      registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), activityResult -> {
        if (activityResult.getResultCode() == Activity.RESULT_OK)
          onDeleteActionSelected(getSelectedCategory());
      });

  private final ActivityResultLauncher<Intent> startImportDirectoryForResult =
      registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), activityResult -> {
        if (activityResult.getResultCode() == Activity.RESULT_OK)
          onImportDirectoryResult(activityResult.getData());
      });

  private final ActivityResultLauncher<Intent> startBookmarkSettingsForResult =
      registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
                                activityResult
                                -> {
                                    // not handled at the moment
                                });

  @Override
  @LayoutRes
  protected int getLayoutRes()
  {
    return R.layout.fragment_bookmark_categories;
  }

  @NonNull
  @Override
  protected BookmarkCategoriesAdapter createAdapter()
  {
    List<BookmarkCategory> items = BookmarkManager.INSTANCE.getCategories();
    return new BookmarkCategoriesAdapter(requireContext(), items);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    getAdapter().setOnClickListener(this);
    getAdapter().setOnLongClickListener(this);
    getAdapter().setOnMoreClickListener(this);
    getAdapter().setCategoryListCallback(this);

    RecyclerView rw = getRecyclerView();
    if (rw == null)
      return;

    rw.setNestedScrollingEnabled(false);
    RecyclerView.ItemDecoration decor = new DividerItemDecorationWithPadding(requireContext(), R.dimen.bookmark_category_margin_start);
    rw.addItemDecoration(decor);
    mCategoriesAdapterObserver = this::onCategoriesChanged;
    BookmarkManager.INSTANCE.addCategoriesUpdatesListener(mCategoriesAdapterObserver);

    setHasOptionsMenu(true);


    ViewGroup toolbar = requireActivity().findViewById(R.id.toolbar);
    mSearchContainer = toolbar.findViewById(R.id.search_container);
    UiUtils.hide(mSearchContainer, R.id.back);

    mToolbarController = new BookmarksToolbarController(toolbar, requireActivity(),
        this::deactivateSearch, this::runSearch, this::cancelSearch);
    mToolbarController.setHint(R.string.search_in_all_lists);


    shareLauncher = SharingUtils.RegisterLauncher(this);
  }

  // ---- Search lifecycle ----

  @Override
  public void onCreateOptionsMenu(@NonNull Menu menu, @NonNull MenuInflater inflater)
  {
    inflater.inflate(R.menu.option_menu_bookmarks, menu);
    inflater.inflate(R.menu.option_menu_bookmark_categories, menu);
  }

  @Override
  public void onPrepareOptionsMenu(@NonNull Menu menu)
  {
    MenuItem itemSearch = menu.findItem(R.id.bookmarks_search);
    itemSearch.setVisible(!mSearchMode);
    MenuItem itemMore = menu.findItem(R.id.bookmarks_more);
    itemMore.setVisible(false);
  }

  @Override
  public boolean onOptionsItemSelected(@NonNull MenuItem item)
  {
    if (item.getItemId() == R.id.bookmarks_search)
    {
      activateSearch();
      return true;
    }
    if (item.getItemId() == R.id.bookmark_categories_sort)
    {
      int currentType = BookmarkManager.INSTANCE.getCategorySortType();
      final int[] sortTypes = {
          BookmarkManager.SORT_CATEGORIES_BY_LAST_MODIFIED,
          BookmarkManager.SORT_CATEGORIES_BY_NAME,
          BookmarkManager.SORT_CATEGORIES_MANUAL
      };
      String[] options = {
          getString(R.string.by_recently_used),
          getString(R.string.by_name),
          getString(R.string.by_custom)
      };

      int checked = currentType;

      new MaterialAlertDialogBuilder(requireActivity())
          .setTitle(R.string.sort_categories)
          .setSingleChoiceItems(options, checked, (dialog, which) -> {
            BookmarkManager.INSTANCE.setCategorySortType(sortTypes[which]);
            dialog.dismiss();
          })
          .show();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  public void activateSearch()
  {
    mSearchMode = true;
    BookmarkManager.INSTANCE.setNotificationsEnabled(true);
    BookmarkManager.INSTANCE.prepareForSearchAll();
    updateSearchVisibility();
  }

  public void deactivateSearch()
  {
    mSearchMode = false;
    BookmarkManager.INSTANCE.setNotificationsEnabled(false);
    BookmarkManager.INSTANCE.releaseSearch();
    updateSearchVisibility();
  }

  public void runSearch(@NonNull String query)
  {
    SearchEngine.INSTANCE.cancel();

    mLastQueryTimestamp = System.nanoTime();
    if (SearchEngine.INSTANCE.searchInBookmarks(query, mLastQueryTimestamp))
      mToolbarController.showProgress(true);
  }

  public void cancelSearch()
  {
    mLastQueryTimestamp = 0;
    SearchEngine.INSTANCE.cancel();
    mToolbarController.showProgress(false);
    updateSearchResults(null);
  }

  @Override
  public void onBookmarkSearchResultsUpdate(@Nullable long[] bookmarkIds, long timestamp)
  {
    if (!isAdded() || !mToolbarController.hasQuery() || mLastQueryTimestamp != timestamp)
      return;
    updateSearchResults(bookmarkIds);
  }

  @Override
  public void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    if (!isAdded() || !mToolbarController.hasQuery() || mLastQueryTimestamp != timestamp)
      return;
    mLastQueryTimestamp = 0;
    mToolbarController.showProgress(false);
    updateSearchResults(bookmarkIds);
  }

  private void updateSearchResults(@Nullable long[] bookmarkIds)
  {
    if (bookmarkIds != null)
    {
      if (mBookmarkListAdapter == null)
      {
        // Dummy category, never displayed for search-all results.
        BookmarkCategory dummy = new BookmarkCategory(0, "", "", "", 0 /* tracksCount */,
                                                      0 /* bookmarksCount */, true /* isVisible */);
        mBookmarkListAdapter = new BookmarkListAdapter(
            new CategoryDataSource(dummy), true /* showCategoryName */);
        mBookmarkListAdapter.setOnClickListener((v, position) -> onSearchResultClick(position));
      }
      mBookmarkListAdapter.setSearchResults(bookmarkIds);
      mBookmarkListAdapter.notifyDataSetChanged();
      getRecyclerView().setAdapter(mBookmarkListAdapter);
    }
    else
    {
      restoreCategoriesAdapter();
    }
    updateRecyclerVisibility();
  }

  private void restoreCategoriesAdapter()
  {
    mBookmarkListAdapter = null;
    getRecyclerView().setAdapter(getAdapter());
  }

  private void updateSearchVisibility()
  {
    if (getAdapter().getItemCount() == 0 && mBookmarkListAdapter == null)
    {
      UiUtils.hide(mSearchContainer);
    }
    else
    {
      UiUtils.showIf(mSearchMode, mSearchContainer);
      if (mSearchMode)
        mToolbarController.activate();
      else
        mToolbarController.deactivate();
    }
    requireActivity().invalidateOptionsMenu();
  }

  private void updateRecyclerVisibility()
  {
    boolean hasResults = mBookmarkListAdapter != null && mBookmarkListAdapter.getItemCount() > 0;
    boolean isEmptySearch = mBookmarkListAdapter != null && mBookmarkListAdapter.getItemCount() == 0;

    if (isEmptySearch)
    {
      requirePlaceholder().setContent(R.string.search_not_found, R.string.search_not_found_query,
                                      R.drawable.ic_search_fail);
      showPlaceholder(true);
    }
    else
    {
      showPlaceholder(false);
    }

    UiUtils.showIf(hasResults || (mBookmarkListAdapter == null), getRecyclerView());
  }

  private void onSearchResultClick(int position)
  {
    BookmarkInfo bookmark = (BookmarkInfo) mBookmarkListAdapter.getItem(position);
    Intent i = new Intent(requireActivity(), MwmActivity.class);
    i.putExtra(MwmActivity.EXTRA_CATEGORY_ID, bookmark.getCategoryId());
    i.putExtra(MwmActivity.EXTRA_BOOKMARK_ID, bookmark.getBookmarkId());
    i.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    startActivity(i);
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    BookmarksSharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), shareLauncher, result);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    SearchEngine.INSTANCE.addBookmarkListener(this);
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addSharingListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    SearchEngine.INSTANCE.removeBookmarkListener(this);
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (mSearchMode)
    {
      cancelSearch();
      deactivateSearch();
    }
    getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    BookmarkManager.INSTANCE.removeCategoriesUpdatesListener(mCategoriesAdapterObserver);
  }


  protected final void showBottomMenu(@NonNull BookmarkCategory item)
  {
    mSelectedCategory = item;
    MenuBottomSheetFragment.newInstance(BOOKMARKS_CATEGORIES_MENU_ID, item.getName())
        .show(getChildFragmentManager(), BOOKMARKS_CATEGORIES_MENU_ID);
  }

  @Override
  @Nullable
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id)
  {
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    if (mSelectedCategory != null)
    {
      items.add(new MenuBottomSheetItem(R.string.edit, R.drawable.ic_edit,
                                        () -> onSettingsActionSelected(mSelectedCategory)));
      items.add(new MenuBottomSheetItem(mSelectedCategory.isVisible() ? R.string.hide : R.string.show,
                                        mSelectedCategory.isVisible() ? R.drawable.ic_hide : R.drawable.ic_show,
                                        () -> onShowActionSelected(mSelectedCategory)));
      items.add(new MenuBottomSheetItem(R.string.export_file, R.drawable.ic_file_kmz,
                                        () -> onShareActionSelected(mSelectedCategory, KmlFileType.Text)));
      items.add(new MenuBottomSheetItem(R.string.export_file_gpx, R.drawable.ic_file_gpx,
                                        () -> onShareActionSelected(mSelectedCategory, KmlFileType.Gpx)));
      // Disallow deleting the last category
      if (getAdapter().getBookmarkCategories().size() > 1)
        items.add(new MenuBottomSheetItem(R.string.delete, R.drawable.ic_delete,
                                          () -> onDeleteActionSelected(mSelectedCategory)));
    }
    return items;
  }

  @Override
  protected void setupPlaceholder(@Nullable PlaceholderView placeholder)
  {
    // Placeholder may be used for search results.
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onBookmarksFileImportFailed()
  {
    // TODO: Is there a way to display several failure notifications?
    // TODO: It would be helpful to see the file name that failed to import.
    final View view = getView();
    // TODO: how to get import button view to show snackbar above it?
    if (view != null)
      Utils.showSnackbar(requireActivity(), view, R.string.load_kmz_failed);
  }

  @Override
  public void onAddButtonClick()
  {
    mCategoryEditor = BookmarkManager.INSTANCE::createCategory;

    EditTextDialogFragment dialogFragment = EditTextDialogFragment.show(
        getString(R.string.bookmarks_create_new_group), getString(R.string.bookmarks_new_list_hint),
        getString(R.string.bookmark_set_name), getString(R.string.create), getString(R.string.cancel),
        MAX_CATEGORY_NAME_LENGTH, this, new CategoryValidator());
    dialogFragment.setTextSaveListener(this::onSaveText);
  }

  @Override
  public void onImportButtonClick()
  {
    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

    // Sic: EXTRA_INITIAL_URI doesn't work
    // https://stackoverflow.com/questions/65326605/extra-initial-uri-will-not-work-no-matter-what-i-do
    // intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, initial);

    // Enable "Show SD card option"
    // http://stackoverflow.com/a/31334967/1615876
    intent.putExtra("android.content.extra.SHOW_ADVANCED", true);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
      intent.putExtra(DocumentsContract.EXTRA_EXCLUDE_SELF, true);

    PackageManager packageManager = requireActivity().getPackageManager();
    if (intent.resolveActivity(packageManager) != null)
      startImportDirectoryForResult.launch(intent);
    else
      showNoFileManagerError();
  }

  private void showNoFileManagerError()
  {
    new MaterialAlertDialogBuilder(requireActivity())
        .setMessage(R.string.error_no_file_manager_app)
        .setPositiveButton(android.R.string.ok, (dialog, which) -> dialog.dismiss())
        .show();
  }

  @Override
  public void onItemClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    mSelectedCategory = category;
    BookmarkListActivity.startForResult(this, startBookmarkListForResult, category);
  }

  @Override
  public void onExportButtonClick()
  {
    BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoriesForSharing(requireActivity());
  }

  private void onShowActionSelected(@NonNull BookmarkCategory category)
  {
    BookmarkManager.INSTANCE.toggleCategoryVisibility(category);
    getAdapter().notifyDataSetChanged();
  }

  protected void onShareActionSelected(@NonNull BookmarkCategory category, KmlFileType kmlFileType)
  {
    BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoryForSharing(requireActivity(), category.getId(), kmlFileType);
  }

  private void onDeleteActionSelected(@NonNull BookmarkCategory category)
  {
    BookmarkManager.INSTANCE.deleteCategory(category.getId());
    getAdapter().notifyDataSetChanged();
  }

  private void onSettingsActionSelected(@NonNull BookmarkCategory category)
  {
    BookmarkCategorySettingsActivity.startForResult(this, startBookmarkSettingsForResult, category);
  }

  private void onImportDirectoryResult(Intent data)
  {
    if (data == null)
      throw new AssertionError("Data is null");

    final Context context = requireActivity();
    final Uri rootUri = data.getData();
    final ProgressDialog dialog = new ProgressDialog(context, R.style.MwmTheme_ProgressDialog);
    dialog.setMessage(getString(R.string.wait_several_minutes));
    dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    dialog.setIndeterminate(true);
    dialog.setCancelable(false);
    dialog.show();
    Logger.d(TAG, "Importing bookmarks from " + rootUri);
    MwmApplication app = MwmApplication.from(context);
    final File tempDir = new File(StorageUtils.getTempPath(app));
    final ContentResolver resolver = context.getContentResolver();
    ThreadPool.getStorage().execute(() -> {
      AtomicInteger found = new AtomicInteger(0);
      StorageUtils.listContentProviderFilesRecursively(resolver, rootUri, uri -> {
        if (BookmarkManager.INSTANCE.importBookmarksFile(resolver, uri, tempDir))
          found.incrementAndGet();
      });
      UiThread.run(() -> {
        if (dialog.isShowing())
          dialog.dismiss();
        int found_val = found.get();
        String message =
            context.getResources().getQuantityString(R.plurals.bookmarks_detect_message, found_val, found_val);
        Toast.makeText(requireContext(), message, Toast.LENGTH_LONG).show();
      });
    });
  }

  @Override
  public void onItemLongClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    showBottomMenu(category);
  }

  public void onItemMoreClick(@NonNull View v, @NonNull BookmarkCategory category)
  {
    showBottomMenu(category);
  }

  private void onSaveText(@NonNull String text)
  {
    if (mCategoryEditor != null)
      mCategoryEditor.commit(text);

    getAdapter().notifyDataSetChanged();
  }

  @NonNull
  protected BookmarkCategory getSelectedCategory()
  {
    if (mSelectedCategory == null)
      throw new AssertionError("Invalid attempt to use null selected category.");
    return mSelectedCategory;
  }

  interface CategoryEditor
  {
    void commit(@NonNull String newName);
  }

  private void onCategoriesChanged()
  {
    getAdapter().setItems(BookmarkManager.INSTANCE.getCategories());
  }
}
