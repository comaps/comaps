package app.organicmaps.widget.placepage;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static app.organicmaps.sdk.util.Utils.getLocalizedFeatureType;
import static app.organicmaps.sdk.util.Utils.getTagValueLocalized;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.bookmarks.BookmarksSharingHelper;
import app.organicmaps.bookmarks.ChooseBookmarkCategoryFragment;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.downloader.MapManagerHelper;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.Bookmark;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.bookmarks.data.KmlFileType;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.bookmarks.data.PredefinedColors;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.OhState;
import app.organicmaps.sdk.editor.OpeningHours;
import app.organicmaps.sdk.editor.data.HoursMinutes;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.SensorListener;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.DateUtils;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.widget.placepage.CoordinatesFormat;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.widget.ArrowView;
import app.organicmaps.widget.placepage.sections.PlacePageBookmarkFragment;
import app.organicmaps.widget.placepage.sections.PlacePageChargeSocketsFragment;
import app.organicmaps.widget.placepage.sections.PlacePageLinksFragment;
import app.organicmaps.widget.placepage.sections.PlacePageOpeningHoursFragment;
import app.organicmaps.widget.placepage.sections.PlacePagePhoneFragment;
import app.organicmaps.widget.placepage.sections.PlacePageTrackFragment;
import app.organicmaps.widget.placepage.sections.PlacePageWikipediaFragment;
import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textview.MaterialTextView;
import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class PlacePageView extends Fragment
    implements View.OnClickListener, View.OnLongClickListener, LocationListener, SensorListener, Observer<MapObject>,
               ChooseBookmarkCategoryFragment.Listener, EditBookmarkFragment.EditBookmarkListener,
               MenuBottomSheetFragment.MenuBottomSheetInterface, BookmarkManager.BookmarksSharingListener

{
  private static final String PREF_COORDINATES_FORMAT = "coordinates_format";
  private static final String BOOKMARK_FRAGMENT_TAG = "BOOKMARK_FRAGMENT_TAG";
  private static final String TRACK_FRAGMENT_TAG = "TRACK_FRAGMENT_TAG";
  private static final String WIKIPEDIA_FRAGMENT_TAG = "WIKIPEDIA_FRAGMENT_TAG";
  private static final String CHARGE_SOCKETS_FRAGMENT_TAG = "CHARGE_SOCKETS_FRAGMENT_TAG";
  private static final String PHONE_FRAGMENT_TAG = "PHONE_FRAGMENT_TAG";
  private static final String OPENING_HOURS_FRAGMENT_TAG = "OPENING_HOURS_FRAGMENT_TAG";
  private static final String LINKS_FRAGMENT_TAG = "LINKS_FRAGMENT_TAG";
  private static final String TRACK_SHARE_MENU_ID = "TRACK_SHARE_MENU_ID";

  private static final List<CoordinatesFormat> visibleCoordsFormat =
      Arrays.asList(CoordinatesFormat.LatLonDMS, CoordinatesFormat.LatLonDecimal, CoordinatesFormat.OLCFull,
                    CoordinatesFormat.UTM, CoordinatesFormat.MGRS, CoordinatesFormat.OSMLink);
  private View mFrame;

  // Preview.
  private ViewGroup mPreview;
  private MaterialToolbar mToolbar;
  private MaterialTextView mTvTitle;
  private MaterialTextView mTvSecondaryTitle;
  private MaterialTextView mTvSubtitle;
  private MaterialTextView mTvOpenState;
  private ArrowView mAvDirection;
  private MaterialTextView mTvDistance;
  private MaterialTextView mTvAddress;
  // Details.
  private MaterialTextView mTvLatlon;
  private View mWifi;
  private MaterialTextView mTvWiFi;
  private View mOperator;
  private MaterialTextView mTvOperator;
  private View mNetwork;
  private MaterialTextView mTvNetwork;
  private View mLevel;
  private MaterialTextView mTvLevel;
  private View mAtm;
  private MaterialTextView mTvAtm;
  private View mCapacity;
  private MaterialTextView mTvCapacity;
  private View mWheelchair;
  private MaterialTextView mTvWheelchair;
  private View mDriveThrough;
  private MaterialTextView mTvDriveThrough;
  private View mSelfService;
  private MaterialTextView mTvSelfService;
  private View mCuisine;
  private MaterialTextView mTvCuisine;
  private View mOutdoorSeating;
  private MaterialTextView mTvOutdoorSeating;
  private View mEntrance;
  private MaterialTextView mTvEntrance;
  private MaterialTextView mTvLastChecked;
  private View mEditPlace;
  private View mAddPlace;
  private View mEditTopSpace;
  private ShapeableImageView mColorIcon;
  private MaterialTextView mTvCategory;
  private MaterialButton mEditBookmark;

  // Data
  private CoordinatesFormat mCoordsFormat = CoordinatesFormat.LatLonDecimal;
  // Downloader`s stuff
  private DownloaderStatusIcon mDownloaderIcon;
  private MaterialTextView mDownloaderInfo;
  private ActivityResultLauncher<SharingUtils.SharingIntent> shareLauncher;
  private int mStorageCallbackSlot;
  @Nullable
  private CountryItem mCurrentCountry;
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      if (mCurrentCountry == null)
        return;

      for (MapManager.StorageCallbackData item : data)
        if (mCurrentCountry.id.equals(item.countryId))
        {
          updateDownloader();
          return;
        }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mCurrentCountry != null && mCurrentCountry.id.equals(countryId))
        updateDownloader();
    }
  };
  private PlacePageViewListener mPlacePageViewListener;

  private PlacePageViewModel mViewModel;
  private MapObject mMapObject;

  private static void refreshMetadataOrHide(@Nullable String metadata, @NonNull View metaLayout,
                                            @NonNull MaterialTextView metaTv)
  {
    if (!TextUtils.isEmpty(metadata))
    {
      metaLayout.setVisibility(VISIBLE);
      metaTv.setText(metadata);
    }
    else
      metaLayout.setVisibility(GONE);
  }

  private static boolean isInvalidDownloaderStatus(int status)
  {
    return (status != CountryItem.STATUS_DOWNLOADABLE && status != CountryItem.STATUS_ENQUEUED
            && status != CountryItem.STATUS_FAILED && status != CountryItem.STATUS_PARTLY
            && status != CountryItem.STATUS_PROGRESS && status != CountryItem.STATUS_APPLYING);
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    shareLauncher = SharingUtils.RegisterLauncher(this);
    return inflater.inflate(R.layout.place_page, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mCoordsFormat =
        CoordinatesFormat.fromId(MwmApplication.prefs(requireContext())
                                     .getInt(PREF_COORDINATES_FORMAT, CoordinatesFormat.LatLonDecimal.getId()));

    Fragment parentFragment = getParentFragment();
    mPlacePageViewListener = (PlacePageViewListener) parentFragment;

    mFrame = view;
    mFrame.setOnClickListener((v) -> mPlacePageViewListener.onPlacePageRequestToggleState());
    mPreview = mFrame.findViewById(R.id.pp__preview);

    mFrame.addOnLayoutChangeListener((v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
      final int oldHeight = oldBottom - oldTop;
      final int newHeight = bottom - top;

      if (oldHeight != newHeight)
        mPlacePageViewListener.onPlacePageContentChanged(mPreview.getHeight(), newHeight);
    });

    mTvTitle = mPreview.findViewById(R.id.tv__title);
    mTvTitle.setOnLongClickListener(this);
    mTvTitle.setOnClickListener(this);
    mTvSecondaryTitle = mPreview.findViewById(R.id.tv__secondary_title);
    mTvSecondaryTitle.setOnLongClickListener(this);
    mTvSecondaryTitle.setOnClickListener(this);
    mToolbar = mFrame.findViewById(R.id.toolbar);
    mTvSubtitle = mPreview.findViewById(R.id.tv__subtitle);
    mTvOpenState = mPreview.findViewById(R.id.tv__open_state);

    View directionFrame = mPreview.findViewById(R.id.direction_frame);
    mTvDistance = mPreview.findViewById(R.id.tv__straight_distance);
    mAvDirection = mPreview.findViewById(R.id.av__direction);
    UiUtils.hide(mTvDistance);
    UiUtils.hide(mAvDirection);
    directionFrame.setOnClickListener(this);

    mTvAddress = mPreview.findViewById(R.id.tv__address);
    mTvAddress.setOnLongClickListener(this);
    mTvAddress.setOnClickListener(this);

    mColorIcon = mFrame.findViewById(R.id.item_icon);
    mTvCategory = mFrame.findViewById(R.id.tv__category);
    mEditBookmark = mFrame.findViewById(R.id.edit_Bookmark);
    mColorIcon.setOnClickListener(this);
    mTvCategory.setOnClickListener(this);
    mEditBookmark.setOnClickListener(this);

    MaterialButton shareButton = mPreview.findViewById(R.id.share_button);
    shareButton.setOnClickListener(this::shareClickListener);

    final MaterialButton closeButton = mPreview.findViewById(R.id.close_button);
    closeButton.setOnClickListener((v) -> mPlacePageViewListener.onPlacePageRequestClose());

    RelativeLayout address = mFrame.findViewById(R.id.ll__place_name);

    LinearLayout latlon = mFrame.findViewById(R.id.ll__place_latlon);
    latlon.setOnClickListener(this);
    LinearLayout openIn = mFrame.findViewById(R.id.ll__place_open_in);
    openIn.setOnClickListener(this);
    openIn.setOnLongClickListener(this);
    openIn.setVisibility(VISIBLE);
    mTvLatlon = mFrame.findViewById(R.id.tv__place_latlon);
    mWifi = mFrame.findViewById(R.id.ll__place_wifi);
    mTvWiFi = mFrame.findViewById(R.id.tv__place_wifi);
    mOutdoorSeating = mFrame.findViewById(R.id.ll__place_outdoor_seating);
    mTvOutdoorSeating = mFrame.findViewById(R.id.tv__place_outdoor_seating);
    mOperator = mFrame.findViewById(R.id.ll__place_operator);
    mOperator.setOnClickListener(this);
    mTvOperator = mFrame.findViewById(R.id.tv__place_operator);
    mNetwork = mFrame.findViewById(R.id.ll__place_network);
    mTvNetwork = mFrame.findViewById(R.id.tv__place_network);
    mLevel = mFrame.findViewById(R.id.ll__place_level);
    mTvLevel = mFrame.findViewById(R.id.tv__place_level);
    mAtm = mFrame.findViewById(R.id.ll__place_atm);
    mTvAtm = mFrame.findViewById(R.id.tv__place_atm);
    mCapacity = mFrame.findViewById(R.id.ll__place_capacity);
    mTvCapacity = mFrame.findViewById(R.id.tv__place_capacity);
    mWheelchair = mFrame.findViewById(R.id.ll__place_wheelchair);
    mTvWheelchair = mFrame.findViewById(R.id.tv__place_wheelchair);
    mDriveThrough = mFrame.findViewById(R.id.ll__place_drive_through);
    mTvDriveThrough = mFrame.findViewById(R.id.tv__place_drive_through);
    mSelfService = mFrame.findViewById(R.id.ll__place_self_service);
    mTvSelfService = mFrame.findViewById(R.id.tv__place_self_service);
    mCuisine = mFrame.findViewById(R.id.ll__place_cuisine);
    mTvCuisine = mFrame.findViewById(R.id.tv__place_cuisine);
    mEntrance = mFrame.findViewById(R.id.ll__place_entrance);
    mTvEntrance = mEntrance.findViewById(R.id.tv__place_entrance);
    mTvLastChecked = mFrame.findViewById(R.id.place_page_last_checked);
    mEditPlace = mFrame.findViewById(R.id.ll__place_editor);
    mAddPlace = mFrame.findViewById(R.id.ll__place_add);
    mEditTopSpace = mFrame.findViewById(R.id.edit_top_space);
    latlon.setOnLongClickListener(this);
    address.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mNetwork.setOnLongClickListener(this);
    mLevel.setOnLongClickListener(this);
    mAtm.setOnLongClickListener(this);
    mCapacity.setOnLongClickListener(this);
    mWheelchair.setOnLongClickListener(this);
    mDriveThrough.setOnLongClickListener(this);
    mSelfService.setOnLongClickListener(this);
    mOutdoorSeating.setOnLongClickListener(this);

    mDownloaderIcon = new DownloaderStatusIcon(mPreview.findViewById(R.id.downloader_status_frame));

    mDownloaderInfo = mPreview.findViewById(R.id.tv__downloader_details);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
    BookmarkManager.INSTANCE.addSharingListener(this);
    MwmApplication.from(requireContext()).getLocationHelper().addListener(this);
    MwmApplication.from(requireContext()).getSensorHelper().addListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
    MwmApplication.from(requireContext()).getLocationHelper().removeListener(this);
    MwmApplication.from(requireContext()).getSensorHelper().removeListener(this);
    UiThread.cancelDelayedTasks(updateOpenState);
    detachCountry();
  }

  private void setCurrentCountry()
  {
    if (mCurrentCountry != null)
      return;
    String country = MapManager.nativeGetSelectedCountry();
    if (country != null && !RoutingController.get().isNavigating())
      attachCountry(country);
  }

  private void refreshViews()
  {
    refreshPreview();
    refreshDetails();
    final Location loc = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
    if (mMapObject.isMyPosition())
      refreshMyPosition(loc);
    else
      refreshDistanceToObject(loc);
    UiUtils.hideIf(mMapObject.isTrack(), mFrame.findViewById(R.id.ll__place_latlon),
                   mFrame.findViewById(R.id.ll__place_open_in));
    if (mMapObject.isTrack())
    {
      UiUtils.hide(mTvSubtitle);
      UiUtils.hide(mAvDirection, mTvDistance);
    }
  }

  private <T extends Fragment> void updateViewFragment(Class<T> controllerClass, String fragmentTag,
                                                       @IdRes int containerId, boolean enabled)
  {
    final FragmentManager fm = getChildFragmentManager();
    final Fragment fragment = fm.findFragmentByTag(fragmentTag);
    if (enabled && fragment == null)
    {
      fm.beginTransaction().setReorderingAllowed(true).add(containerId, controllerClass, null, fragmentTag).commit();
    }
    else if (!enabled && fragment != null)
    {
      fm.beginTransaction().setReorderingAllowed(true).remove(fragment).commit();
    }
  }

  private void updateLinksView()
  {
    updateViewFragment(PlacePageLinksFragment.class, LINKS_FRAGMENT_TAG, R.id.place_page_links_fragment, true);
  }

  private void updateOpeningHoursView()
  {
    final String ohStr = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    updateViewFragment(PlacePageOpeningHoursFragment.class, OPENING_HOURS_FRAGMENT_TAG,
                       R.id.place_page_opening_hours_fragment, !TextUtils.isEmpty(ohStr));
  }

  private void updateChargeSocketsView()
  {
    updateViewFragment(PlacePageChargeSocketsFragment.class, CHARGE_SOCKETS_FRAGMENT_TAG,
                       R.id.place_page_charge_sockets_fragment, mMapObject.hasChargeSockets());
  }

  private void updatePhoneView()
  {
    updateViewFragment(PlacePagePhoneFragment.class, PHONE_FRAGMENT_TAG, R.id.place_page_phone_fragment,
                       mMapObject.hasPhoneNumber());
  }

  private void updateBookmarkView()
  {
    updateViewFragment(PlacePageBookmarkFragment.class, BOOKMARK_FRAGMENT_TAG, R.id.place_page_bookmark_fragment,
                       mMapObject.isBookmark());
  }

  private void updateTrackView()
  {
    updateViewFragment(PlacePageTrackFragment.class, TRACK_FRAGMENT_TAG, R.id.place_page_track_fragment,
                       mMapObject.isTrack());
  }

  private boolean hasWikipediaEntry()
  {
    final String wikipediaLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA);
    final String description = mMapObject.getDescription();
    return !TextUtils.isEmpty(wikipediaLink) || !TextUtils.isEmpty(description);
  }

  private void updateWikipediaView()
  {
    updateViewFragment(PlacePageWikipediaFragment.class, WIKIPEDIA_FRAGMENT_TAG, R.id.place_page_wikipedia_fragment,
                       hasWikipediaEntry());
  }

  private void setTextAndColorizeSubtitle()
  {
    String text = mMapObject.getSubtitle();
    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, text);
    if (!TextUtils.isEmpty(text))
    {
      SpannableStringBuilder sb = new SpannableStringBuilder(text);
      int start = text.indexOf("★");
      int end = text.lastIndexOf("★") + 1;
      if (start > -1)
      {
        sb.setSpan(new ForegroundColorSpan(ContextCompat.getColor(requireContext(), R.color.base_yellow)), start, end,
                   Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
      }
      mTvSubtitle.setText(sb);
    }
  }

  private void refreshPreview()
  {
    UiUtils.setTextAndHideIfEmpty(mTvTitle, mMapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSecondaryTitle, mMapObject.getSecondaryTitle());
    refreshOpenState();

    if (mToolbar != null)
      mToolbar.setTitle(mMapObject.getTitle());
    setTextAndColorizeSubtitle();
    UiUtils.setTextAndHideIfEmpty(mTvAddress, mMapObject.getAddress());
    refreshCategoryPreview();
  }

  void refreshCategoryPreview()
  {
    View categoryContainer = mFrame.findViewById(R.id.category_container);
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      Drawable circle =
          Graphics.drawCircle(track.getColor(), R.dimen.place_page_icon_size, requireContext().getResources());
      mColorIcon.setImageDrawable(circle);
      mTvCategory.setText(BookmarkManager.INSTANCE.getCategoryById(track.getCategoryId()).getName());
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      Icon icon = bookmark.getIcon();
      if (icon != null)
      {
        Drawable circle = Graphics.drawCircleAndImage(icon.argb(), R.dimen.place_page_icon_size, icon.getResId(),
                                                      R.dimen.place_page_icon_mark_size, requireContext());
        mColorIcon.setImageDrawable(circle);
        mTvCategory.setText(BookmarkManager.INSTANCE.getCategoryById(bookmark.getCategoryId()).getName());
      }
    }
    UiUtils.showIf(mMapObject.isTrack() || mMapObject.isBookmark(), categoryContainer);
  }

  void showColorDialog()
  {
    final Bundle args = new Bundle();
    final FragmentManager manager = getChildFragmentManager();
    String className = BookmarkColorDialogFragment.class.getName();
    final FragmentFactory factory = manager.getFragmentFactory();
    final BookmarkColorDialogFragment dialogFragment =
        (BookmarkColorDialogFragment) factory.instantiate(getContext().getClassLoader(), className);
    dialogFragment.setArguments(args);

    if (mMapObject.isTrack())
    {
      final Track track = (Track) mMapObject;
      args.putInt(BookmarkColorDialogFragment.ICON_COLOR, PredefinedColors.getPredefinedColorIndex(track.getColor()));
      dialogFragment.setOnColorSetListener((colorPos) -> {
        int from = track.getColor();
        int to = PredefinedColors.getColor(colorPos);
        if (from == to)
          return;
        track.setColor(to);
        Drawable circle = Graphics.drawCircle(to, R.dimen.place_page_icon_size, requireContext().getResources());
        mColorIcon.setImageDrawable(circle);
      });
      dialogFragment.show(requireActivity().getSupportFragmentManager(), null);
    }
    else if (mMapObject.isBookmark())
    {
      final Bookmark bookmark = (Bookmark) mMapObject;
      args.putInt(BookmarkColorDialogFragment.ICON_COLOR, bookmark.getIcon().getColor());
      args.putInt(BookmarkColorDialogFragment.ICON_RES, bookmark.getIcon().getResId());
      dialogFragment.setOnColorSetListener((colorPos) -> {
        int from = bookmark.getIcon().argb();
        int to = PredefinedColors.getColor(colorPos);
        if (from == to)
          return;
        bookmark.setIconColor(to);
        Drawable circle = Graphics.drawCircleAndImage(to, R.dimen.place_page_icon_size, bookmark.getIcon().getResId(),
                                                      R.dimen.place_page_icon_mark_size, requireContext());
        mColorIcon.setImageDrawable(circle);
      });
      dialogFragment.show(requireActivity().getSupportFragmentManager(), null);
    }
  }

  private void showCategoryList()
  {
    final Bundle args = new Bundle();
    final List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
    final FragmentManager manager = getChildFragmentManager();
    String className = ChooseBookmarkCategoryFragment.class.getName();
    final FragmentFactory factory = manager.getFragmentFactory();
    final ChooseBookmarkCategoryFragment frag =
        (ChooseBookmarkCategoryFragment) factory.instantiate(getContext().getClassLoader(), className);
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      BookmarkCategory currentCategory = BookmarkManager.INSTANCE.getCategoryById(track.getCategoryId());
      final int index = categories.indexOf(currentCategory);
      args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_POSITION, index);
      frag.setArguments(args);
      frag.show(manager, null);
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      BookmarkCategory currentCategory = BookmarkManager.INSTANCE.getCategoryById(bookmark.getCategoryId());
      final int index = categories.indexOf(currentCategory);
      args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_POSITION, index);
      frag.setArguments(args);
      frag.show(manager, null);
    }
  }

  @Override
  public void onCategoryChanged(@NonNull BookmarkCategory newCategory)
  {
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      BookmarkCategory previousCategory = BookmarkManager.INSTANCE.getCategoryById(track.getCategoryId());
      if (previousCategory == newCategory)
        return;
      BookmarkManager.INSTANCE.notifyCategoryChanging(track, newCategory.getId());
      mTvCategory.setText(newCategory.getName());
      track.setCategoryId(newCategory.getId());
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      BookmarkCategory previousCategory = BookmarkManager.INSTANCE.getCategoryById(bookmark.getCategoryId());
      if (previousCategory == newCategory)
        return;
      mTvCategory.setText(newCategory.getName());
      bookmark.setCategoryId(newCategory.getId());
    }
  }

  void showBookmarkEditFragment()
  {
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      final FragmentActivity activity = requireActivity();
      EditBookmarkFragment.editTrack(track.getCategoryId(), track.getTrackId(), activity, getChildFragmentManager(),
                                     PlacePageView.this);
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      final FragmentActivity activity = requireActivity();
      EditBookmarkFragment.editBookmark(bookmark.getCategoryId(), bookmark.getBookmarkId(), activity,
                                        getChildFragmentManager(), PlacePageView.this);
    }
  }

  @Override
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    if (mMapObject.isTrack())
      BookmarkManager.INSTANCE.updateTrackPlacePage();
    else if (mMapObject.isBookmark())
      BookmarkManager.INSTANCE.updateBookmarkPlacePage(bookmarkId);
  }

  private void refreshDetails()
  {
    refreshLatLon();

    final String operator = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR);
    refreshMetadataOrHide(!TextUtils.isEmpty(operator) ? getString(R.string.operator, operator) : "", mOperator,
                          mTvOperator);

    final String network = mMapObject.getMetadata(Metadata.MetadataType.FMD_NETWORK);
    refreshMetadataOrHide(!TextUtils.isEmpty(network) ? getString(R.string.network, network) : "", mNetwork,
                          mTvNetwork);

    /// @todo I don't like it when we take all data from mapObject, but for cuisines, we should
    /// go into JNI Framework and rely on some "active object".
    refreshMetadataOrHide(Framework.nativeGetActiveObjectFormattedCuisine(), mCuisine, mTvCuisine);
    refreshWiFi();
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    final String level = Utils.getLocalizedLevel(getContext(), mMapObject.getMetadata(Metadata.MetadataType.FMD_LEVEL));
    refreshMetadataOrHide(level, mLevel, mTvLevel);

    final String cap = mMapObject.getMetadata(Metadata.MetadataType.FMD_CAPACITY);
    refreshMetadataOrHide(!TextUtils.isEmpty(cap) ? getString(R.string.capacity, cap) : "", mCapacity, mTvCapacity);

    refreshMetadataOrHide(mMapObject.hasAtm() ? getString(app.organicmaps.sdk.R.string.type_amenity_atm) : "", mAtm,
                          mTvAtm);

    final String wheelchair =
        getLocalizedFeatureType(getContext(), mMapObject.getMetadata(Metadata.MetadataType.FMD_WHEELCHAIR));
    refreshMetadataOrHide(wheelchair, mWheelchair, mTvWheelchair);

    final String driveThrough = mMapObject.getMetadata(Metadata.MetadataType.FMD_DRIVE_THROUGH);
    refreshMetadataOrHide(driveThrough.equals("yes") ? getString(R.string.drive_through) : "", mDriveThrough,
                          mTvDriveThrough);

    final String selfService = mMapObject.getMetadata(Metadata.MetadataType.FMD_SELF_SERVICE);
    refreshMetadataOrHide(getTagValueLocalized(getContext(), "self_service", selfService), mSelfService,
                          mTvSelfService);

    final String outdoorSeating = mMapObject.getMetadata(Metadata.MetadataType.FMD_OUTDOOR_SEATING);
    refreshMetadataOrHide(outdoorSeating.equals("yes") ? getString(R.string.outdoor_seating) : "", mOutdoorSeating,
                          mTvOutdoorSeating);

    final String lastChecked = mMapObject.getMetadata(Metadata.MetadataType.FMD_CHECK_DATE);
    if (!lastChecked.isEmpty())
    {
      String periodSinceCheck = DateUtils.getRelativePeriodString(getResources(), lastChecked);
      UiUtils.setTextAndShow(mTvLastChecked,
                             requireContext().getString(R.string.existence_confirmed_time_ago, periodSinceCheck));
    }
    else
      UiUtils.hide(mTvLastChecked);

    if (RoutingController.get().isNavigating() || RoutingController.get().isPlanning())
    {
      UiUtils.hide(mEditPlace, mAddPlace, mEditTopSpace);
    }
    else
    {
      UiUtils.showIf(Editor.nativeShouldShowEditPlace(), mEditPlace);
      UiUtils.showIf(Editor.nativeShouldShowAddPlace(), mAddPlace);
      MaterialButton mTvEditPlace = mEditPlace.findViewById(R.id.mb__place_editor);
      MaterialButton mTvAddPlace = mAddPlace.findViewById(R.id.mb__place_add);
      mTvEditPlace.setOnClickListener(this);
      mTvAddPlace.setOnClickListener(this);
      mTvEditPlace.setEnabled(Editor.nativeShouldEnableEditPlace());
      mTvAddPlace.setEnabled(Editor.nativeShouldEnableAddPlace());
      final int editTextButtonColor =
          Editor.nativeShouldEnableEditPlace()
              ? ContextCompat.getColor(
                    getContext(),
                    UiUtils.getStyledResourceId(getContext(), com.google.android.material.R.attr.colorSecondary))
              : ContextCompat.getColor(getContext(), R.color.button_accent_text_disabled);
      final ColorStateList editStrokeButtonColor = new ColorStateList(
              new int[][]{
                      new int[]{android.R.attr.state_enabled}, // enabled
                      new int[]{-android.R.attr.state_enabled} // disabled
      },
              new int[]{
                      ContextCompat.getColor(
                              getContext(),
                              UiUtils.getStyledResourceId(getContext(), com.google.android.material.R.attr.colorSecondary)),
                      ContextCompat.getColor(getContext(), R.color.button_accent_text_disabled)
              });
      mTvEditPlace.setTextColor(editTextButtonColor);
      mTvAddPlace.setTextColor(editTextButtonColor);
      mTvEditPlace.setStrokeColor(editStrokeButtonColor);
      mTvAddPlace.setStrokeColor(editStrokeButtonColor);
      UiUtils.showIf(
          UiUtils.isVisible(mEditPlace) || UiUtils.isVisible(mAddPlace),
          mEditTopSpace);
    }
    updateLinksView();
    updateOpeningHoursView();
    updateWikipediaView();
    updateBookmarkView();
    updateChargeSocketsView();
    updatePhoneView();
    updateTrackView();
  }

  private void refreshWiFi()
  {
    final String inet = mMapObject.getMetadata(Metadata.MetadataType.FMD_INTERNET);
    if (!TextUtils.isEmpty(inet))
    {
      mWifi.setVisibility(VISIBLE);
      /// @todo Better (but harder) to wrap C++ osm::Internet into Java, instead of comparing with "no".
      mTvWiFi.setText(TextUtils.equals(inet, "no") ? R.string.no_available : R.string.yes_available);
    }
    else
      mWifi.setVisibility(GONE);
  }

  private void refreshMyPosition(Location l)
  {
    UiUtils.hide(mTvDistance);
    UiUtils.hide(mAvDirection);

    if (l == null)
      return;

    final StringBuilder builder = new StringBuilder();
    if (l.hasAltitude())
      builder.append("▲").append(Framework.nativeFormatAltitude(l.getAltitude()));
    if (l.hasSpeed())
      builder.append("   ").append(Framework.nativeFormatSpeed(l.getSpeed()));

    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, builder.toString());

    mMapObject.setLat(l.getLatitude());
    mMapObject.setLon(l.getLongitude());
    refreshLatLon();
  }

  private void refreshDistanceToObject(Location l)
  {
    if (mMapObject.isTrack())
      return;
    UiUtils.showIf(l != null, mTvDistance);
    if (l == null)
      return;

    double lat = mMapObject.getLat();
    double lon = mMapObject.getLon();
    DistanceAndAzimut distanceAndAzimuth =
        Framework.nativeGetDistanceAndAzimuthFromLatLon(lat, lon, l.getLatitude(), l.getLongitude(), 0.0);
    mTvDistance.setText(distanceAndAzimuth.getDistance().toString(requireContext()));
  }

  private void refreshLatLon()
  {
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    String latLon = Framework.nativeFormatLatLon(lat, lon, mCoordsFormat.getId());
    if (latLon == null) // Some coordinates couldn't be converted to UTM and MGRS
      latLon = "N/A";

    if (mCoordsFormat.showLabel())
      mTvLatlon.setText(mCoordsFormat.getLabel() + ": " + latLon);
    else
      mTvLatlon.setText(latLon);
  }

  Runnable updateOpenState = this::refreshOpenState;

  private void refreshOpenState()
  {
    UiThread.runLater(updateOpenState, 45000); // Refresh every 45s

    final String ohStr = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);

    if (timetables != null && timetables.length != 0)
    {
      final Context context = requireContext();
      final OhState poiState = OpeningHours.nativeCurrentState(timetables);

      // Ignore unknown rule state
      if (poiState.state == OhState.State.Unknown)
      {
        UiUtils.hide(mTvOpenState);
        return;
      }

      // Get colours
      final ForegroundColorSpan colorGreen =
          new ForegroundColorSpan(ContextCompat.getColor(context, R.color.base_green));
      final ForegroundColorSpan colorYellow =
          new ForegroundColorSpan(ContextCompat.getColor(context, R.color.base_yellow));
      final ForegroundColorSpan colorRed = new ForegroundColorSpan(ContextCompat.getColor(context, R.color.base_red));

      // Get next state info
      final SpannableStringBuilder openStateString = new SpannableStringBuilder();
      final boolean isOpen = (poiState.state == OhState.State.Open); // False == Closed due to early exit for Unknown
      final long nextStateTime = isOpen ? poiState.nextTimeClosed : poiState.nextTimeOpen; // Unix time (seconds)
      final int minsToNextState = (int) ((nextStateTime - (System.currentTimeMillis() / 1000)) / 60);

      if (minsToNextState <= 60) // POI opens/closes in 60 mins
      {
        final String minsToChangeStr = minsToNextState + " " + getString(R.string.minute);
        final String nextChangeFormatted = getString(isOpen ? R.string.closes_in : R.string.opens_in, minsToChangeStr);
        final ForegroundColorSpan nextChangeColor = isOpen ? colorYellow : colorRed;
        // TODO: We should check closed/open time for specific feature's timezone.
        ZonedDateTime time = ZonedDateTime.ofInstant(Instant.ofEpochSecond(nextStateTime), ZoneId.systemDefault());
        String localizedTime =
            new HoursMinutes(time.getHour(), time.getMinute(), DateUtils.is24HourFormat(context)).toString();

        openStateString.append(nextChangeFormatted, nextChangeColor, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
            .append(" • ") // Add spacer
            .append(getString(R.string.at, localizedTime));
      }
      else if (isOpen)
        openStateString.append(getString(R.string.open_now), colorGreen, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      // TODO: Add "Closes at 18:00" etc
      else // Closed
        openStateString.append(getString(R.string.closed_now), colorRed, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      // TODO: Add "Opens at 18:00" etc

      UiUtils.setTextAndHideIfEmpty(mTvOpenState, openStateString);
      return;
    }
    // No valid timetable
    UiUtils.hide(mTvOpenState);
  }

  private void addPlace()
  {
    ((MwmActivity) requireActivity()).showPositionChooserForEditor(false, true);
  }

  @Override
  public void onClick(View v)
  {
    final Context context = requireContext();
    final int id = v.getId();
    if (id == R.id.tv__title || id == R.id.tv__secondary_title || id == R.id.tv__address)
    {
      // A workaround to make single taps toggle the bottom sheet.
      mPlacePageViewListener.onPlacePageRequestToggleState();
    }
    else if (id == R.id.mb__place_editor)
      ((MwmActivity) requireActivity()).showEditor();
    else if (id == R.id.mb__place_add)
      addPlace();
    else if (id == R.id.ll__place_latlon)
    {
      final int formatIndex = visibleCoordsFormat.indexOf(mCoordsFormat);
      mCoordsFormat = visibleCoordsFormat.get((formatIndex + 1) % visibleCoordsFormat.size());
      MwmApplication.prefs(context).edit().putInt(PREF_COORDINATES_FORMAT, mCoordsFormat.getId()).apply();
      refreshLatLon();
    }
    else if (id == R.id.ll__place_open_in)
    {
      final String uri = Framework.nativeGetGeoUri(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getScale(),
                                                   mMapObject.getName());
      Utils.openUri(requireContext(), Uri.parse(uri), R.string.uri_open_location_failed);
    }
    else if (id == R.id.direction_frame)
      showBigDirection();
    else if (id == R.id.item_icon)
      showColorDialog();
    else if (id == R.id.edit_Bookmark)
      showBookmarkEditFragment();
    else if (id == R.id.tv__category)
      showCategoryList();
  }

  private void showBigDirection()
  {
    final FragmentManager fragmentManager = requireActivity().getSupportFragmentManager();
    final DirectionFragment fragment = (DirectionFragment) fragmentManager.getFragmentFactory().instantiate(
        requireContext().getClassLoader(), DirectionFragment.class.getName());
    fragment.setMapObject(mMapObject);
    fragment.show(fragmentManager, null);
  }

  @Override
  public boolean onLongClick(View v)
  {
    final List<String> items = new ArrayList<>();
    final int id = v.getId();
    if (id == R.id.tv__title)
      items.add(mTvTitle.getText().toString());
    else if (id == R.id.tv__secondary_title)
      items.add(mTvSecondaryTitle.getText().toString());
    else if (id == R.id.tv__address)
      items.add(mTvAddress.getText().toString());
    else if (id == R.id.ll__place_latlon)
    {
      final double lat = mMapObject.getLat();
      final double lon = mMapObject.getLon();
      for (CoordinatesFormat format : visibleCoordsFormat)
      {
        String formatted = Framework.nativeFormatLatLon(lat, lon, format.getId());
        if (formatted != null)
          items.add(formatted);
      }
    }
    else if (id == R.id.ll__place_open_in)
    {
      final String uri = Framework.nativeGetGeoUri(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getScale(),
                                                   mMapObject.getName());
      PlacePageUtils.copyToClipboard(requireContext(), mFrame, uri);
    }
    else if (id == R.id.ll__place_operator)
      items.add(mTvOperator.getText().toString());
    else if (id == R.id.ll__place_network)
      items.add(mTvNetwork.getText().toString());
    else if (id == R.id.ll__place_level)
      items.add(mTvLevel.getText().toString());
    else if (id == R.id.ll__place_atm)
      items.add(mTvAtm.getText().toString());
    else if (id == R.id.ll__place_capacity)
      items.add(mTvCapacity.getText().toString());
    else if (id == R.id.ll__place_wheelchair)
      items.add(mTvWheelchair.getText().toString());
    else if (id == R.id.ll__place_drive_through)
      items.add(mTvDriveThrough.getText().toString());
    else if (id == R.id.ll__place_outdoor_seating)
      items.add(mTvOutdoorSeating.getText().toString());

    final Context context = requireContext();
    if (items.size() == 1)
      PlacePageUtils.copyToClipboard(context, mFrame, items.get(0));
    else
      PlacePageUtils.showCopyPopup(context, v, items);

    return true;
  }

  private void updateDownloader(CountryItem country)
  {
    if (isInvalidDownloaderStatus(country.status))
    {
      if (mStorageCallbackSlot != 0)
        UiThread.runLater(this::detachCountry);
      return;
    }

    mDownloaderIcon.update(country);

    StringBuilder sb = new StringBuilder(StringUtils.getFileSizeString(requireContext(), country.totalSize));
    if (country.isExpandable())
      sb.append(StringUtils.formatUsingUsLocale(
          "  •  %s: %d", requireContext().getString(R.string.downloader_status_maps), country.totalChildCount));

    mDownloaderInfo.setText(sb.toString());
  }

  private void updateDownloader()
  {
    if (mCurrentCountry == null)
      return;

    mCurrentCountry.update();
    updateDownloader(mCurrentCountry);
  }

  private void attachCountry(String country)
  {
    CountryItem map = CountryItem.fill(country);
    if (isInvalidDownloaderStatus(map.status))
      return;

    mCurrentCountry = map;
    if (mStorageCallbackSlot == 0)
      mStorageCallbackSlot = MapManager.nativeSubscribe(mStorageCallback);

    mDownloaderIcon
        .setOnIconClickListener((v) -> MapManagerHelper.warn3gAndDownload(requireActivity(), mCurrentCountry.id, null))
        .setOnCancelClickListener((v) -> MapManager.nativeCancel(mCurrentCountry.id));
    mDownloaderIcon.show(true);
    UiUtils.show(mDownloaderInfo);
    updateDownloader(mCurrentCountry);
  }

  private void detachCountry()
  {
    if (mStorageCallbackSlot == 0 || mCurrentCountry == null)
      return;

    MapManager.nativeUnsubscribe(mStorageCallbackSlot);
    mStorageCallbackSlot = 0;
    mCurrentCountry = null;
    mDownloaderIcon.setOnIconClickListener(null).setOnCancelClickListener(null);
    mDownloaderIcon.show(false);
    UiUtils.hide(mDownloaderInfo);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject == null)
      return;
    // Starting the download will fire this callback but the object will be the same
    // Detaching the country in that case will crash the app
    if (!mapObject.sameAs(mMapObject))
      detachCountry();
    setCurrentCountry();

    mMapObject = mapObject;

    refreshViews();
    // In case the place page has already some data, make sure to call the onPlacePageContentChanged callback
    // to catch cases where the new data has the exact same height as the previous one (eg 2 address nodes)
    if (mFrame.getHeight() > 0)
      mPlacePageViewListener.onPlacePageContentChanged(mPreview.getHeight(), mFrame.getHeight());
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    if (mMapObject == null)
      return;
    if (mMapObject.isMyPosition())
      refreshMyPosition(location);
    else
      refreshDistanceToObject(location);
  }

  @Override
  public void onCompassUpdated(double north)
  {
    if (mMapObject == null || mMapObject.isMyPosition() || mMapObject.isTrack())
      return;

    final Location location = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
    if (location == null)
    {
      UiUtils.hide(mAvDirection);
      return;
    }

    final double azimuth =
        Framework
            .nativeGetDistanceAndAzimuthFromLatLon(mMapObject.getLat(), mMapObject.getLon(), location.getLatitude(),
                                                   location.getLongitude(), north)
            .getAzimuth();
    UiUtils.showIf(azimuth >= 0, mAvDirection);
    if (azimuth >= 0)
    {
      mAvDirection.setAzimuth(azimuth);
    }
  }

  void shareClickListener(View v)
  {
    if (mMapObject.isTrack())
    {
      MenuBottomSheetFragment.newInstance(TRACK_SHARE_MENU_ID, getString(R.string.share_track))
          .show(getChildFragmentManager(), TRACK_SHARE_MENU_ID);
    }
    else
      SharingUtils.shareMapObject(requireContext(), mMapObject);
  }

  private void onShareTrackSelected(long trackId, KmlFileType kmlFileType)
  {
    BookmarksSharingHelper.INSTANCE.prepareTrackForSharing(requireActivity(), trackId, kmlFileType);
  }

  @Nullable
  @Override
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id)
  {
    return switch (id)
    {
      case TRACK_SHARE_MENU_ID -> getTrackShareMenuItems();
      default -> null;
    };
  }

  public ArrayList<MenuBottomSheetItem> getTrackShareMenuItems()
  {
    Track track = (Track) mMapObject;
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    items.add(new MenuBottomSheetItem(R.string.export_file, R.drawable.ic_file_kmz,
                                      () -> onShareTrackSelected(track.getTrackId(), KmlFileType.Text)));
    items.add(new MenuBottomSheetItem(R.string.export_file_gpx, R.drawable.ic_file_gpx,
                                      () -> onShareTrackSelected(track.getTrackId(), KmlFileType.Gpx)));
    return items;
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    BookmarksSharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), shareLauncher, result);
  }

  public interface PlacePageViewListener
  {
    // Called when the content has actually changed and we are ready to compute the peek height
    void onPlacePageContentChanged(int previewHeight, int frameHeight);

    void onPlacePageRequestToggleState();
    void onPlacePageRequestClose();
  }
}
