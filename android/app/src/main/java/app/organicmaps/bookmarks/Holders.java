package app.organicmaps.bookmarks;

import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.PluralsRes;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.IconClickListener;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.recycler.RecyclerClickListener;
import app.organicmaps.widget.recycler.RecyclerLongClickListener;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.checkbox.MaterialCheckBox;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textview.MaterialTextView;

public class Holders
{
  public static class GeneralViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final MaterialTextView mText;
    @NonNull
    private final ShapeableImageView mImage;

    GeneralViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mImage = itemView.findViewById(R.id.image);
      mText = itemView.findViewById(R.id.text);
    }

    @NonNull
    public MaterialTextView getText()
    {
      return mText;
    }

    @NonNull
    public ShapeableImageView getImage()
    {
      return mImage;
    }
  }

  public static class HeaderViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final MaterialButton mButton;
    @NonNull
    private final MaterialTextView mText;

    HeaderViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mButton = itemView.findViewById(R.id.button);
      mText = itemView.findViewById(R.id.text_message);
    }

    @NonNull
    public MaterialTextView getText()
    {
      return mText;
    }

    @NonNull
    public MaterialButton getButton()
    {
      return mButton;
    }

    void setAction(@NonNull HeaderAction action, final boolean showAll)
    {
      mButton.setText(showAll ? R.string.bookmark_lists_show_all : R.string.bookmark_lists_hide_all);
      mButton.setOnClickListener(new ToggleShowAllClickListener(action, showAll));
    }

    void setAction(@NonNull HeaderActionChildCategories action, final boolean showAll)
    {
      mButton.setText(showAll ? R.string.bookmark_lists_show_all : R.string.bookmark_lists_hide_all);
      mButton.setOnClickListener(new ToggleShowAllChildCategoryClickListener(action, showAll));
    }

    public interface HeaderAction
    {
      void onHideAll();

      void onShowAll();
    }

    public interface HeaderActionChildCategories
    {
      void onHideAll();

      void onShowAll();
    }

    private static class ToggleShowAllChildCategoryClickListener implements View.OnClickListener
    {
      private final HeaderActionChildCategories mAction;
      private final boolean mShowAll;

      ToggleShowAllChildCategoryClickListener(@NonNull HeaderActionChildCategories action, boolean showAll)
      {
        mAction = action;
        mShowAll = showAll;
      }

      @Override
      public void onClick(View view)
      {
        if (mShowAll)
          mAction.onShowAll();
        else
          mAction.onHideAll();
      }
    }

    private static class ToggleShowAllClickListener implements View.OnClickListener
    {
      private final HeaderAction mAction;
      private final boolean mShowAll;

      ToggleShowAllClickListener(@NonNull HeaderAction action, boolean showAll)
      {
        mAction = action;
        mShowAll = showAll;
      }

      @Override
      public void onClick(View view)
      {
        if (mShowAll)
          mAction.onShowAll();
        else
          mAction.onHideAll();
      }
    }
  }

  static class CategoryViewHolderBase extends RecyclerView.ViewHolder
  {
    @Nullable
    protected BookmarkCategory mEntity;

    @NonNull
    protected final MaterialTextView mSize;

    public CategoryViewHolderBase(@NonNull View root)
    {
      super(root);
      mSize = root.findViewById(R.id.size);
    }

    protected void setSize()
    {
      if (mEntity == null)
        return;

      mSize.setText(getSizeString());
    }

    private String getSizeString()
    {
      final Resources resources = mSize.getResources();
      final int bookmarksCount = mEntity.getBookmarksCount();
      final int tracksCount = mEntity.getTracksCount();

      if ((bookmarksCount == 0 && tracksCount == 0) || (bookmarksCount > 0 && tracksCount > 0))
      {
        final String bookmarks = getQuantified(resources, R.plurals.bookmarks_places, bookmarksCount);
        final String tracks = getQuantified(resources, R.plurals.tracks, tracksCount);
        final String template = resources.getString(R.string.comma_separated_pair);

        return String.format(template, bookmarks, tracks);
      }

      if (bookmarksCount > 0)
        return getQuantified(resources, R.plurals.bookmarks_places, bookmarksCount);

      return getQuantified(resources, R.plurals.tracks, tracksCount);
    }

    void setEntity(@NonNull BookmarkCategory entity)
    {
      mEntity = entity;
    }

    @NonNull
    public BookmarkCategory getEntity()
    {
      if (mEntity == null)
        throw new AssertionError("BookmarkCategory is null");
      return mEntity;
    }

    private String getQuantified(Resources resources, @PluralsRes int plural, int size)
    {
      return resources.getQuantityString(plural, size, size);
    }
  }
  static class CollectionViewHolder extends CategoryViewHolderBase
  {
    @NonNull
    private final View mView;
    @NonNull
    private final MaterialTextView mName;
    @NonNull
    private final MaterialCheckBox mVisibilityMarker;

    CollectionViewHolder(@NonNull View root)
    {
      super(root);
      mView = root;
      mName = root.findViewById(R.id.name);
      mVisibilityMarker = root.findViewById(R.id.checkbox);
    }

    void setOnClickListener(@Nullable OnItemClickListener<BookmarkCategory> listener)
    {
      mView.setOnClickListener(v -> {
        if (listener != null && mEntity != null)
          listener.onItemClick(v, mEntity);
      });
    }

    void setVisibilityState(boolean visible)
    {
      mVisibilityMarker.setChecked(visible);
    }

    void setVisibilityListener(@Nullable View.OnClickListener listener)
    {
      mVisibilityMarker.setOnClickListener(listener);
    }

    void setName(@NonNull String name)
    {
      mName.setText(name);
    }
  }

  static class CategoryViewHolder extends CategoryViewHolderBase
  {
    @NonNull
    private final MaterialTextView mName;
    @NonNull
    MaterialCheckBox mVisibilityMarker;
    @NonNull
    ShapeableImageView mMoreButton;

    CategoryViewHolder(@NonNull View root)
    {
      super(root);
      mName = root.findViewById(R.id.name);
      mVisibilityMarker = root.findViewById(R.id.checkbox);
      mMoreButton = root.findViewById(R.id.more);
    }

    void setVisibilityState(boolean visible)
    {
      mVisibilityMarker.setChecked(visible);
    }

    void setVisibilityListener(@Nullable View.OnClickListener listener)
    {
      mVisibilityMarker.setOnClickListener(listener);
    }

    void setMoreButtonClickListener(@Nullable View.OnClickListener listener)
    {
      mMoreButton.setOnClickListener(listener);
    }

    void setName(@NonNull String name)
    {
      mName.setText(name);
    }
  }

  static abstract class BaseBookmarkHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final View mView;

    BaseBookmarkHolder(@NonNull View itemView)
    {
      super(itemView);
      mView = itemView;
    }

    abstract void bind(@NonNull SectionPosition position,
                       @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource);

    void setOnClickListener(@Nullable RecyclerClickListener listener)
    {
      mView.setOnClickListener(v -> {
        if (listener != null)
          listener.onItemClick(v, getBindingAdapterPosition());
      });
    }

    void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
    {
      mView.setOnLongClickListener(v -> {
        if (listener != null)
          listener.onLongItemClick(v, getBindingAdapterPosition());
        return true;
      });
    }
  }

  static class BookmarkViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final ShapeableImageView mIcon;
    @NonNull
    private final MaterialTextView mName;
    @NonNull
    private final MaterialTextView mDistance;

    BookmarkViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      final long bookmarkId = sectionsDataSource.getBookmarkId(position);
      BookmarkInfo bookmark = new BookmarkInfo(sectionsDataSource.getCategory().getId(), bookmarkId);
      mName.setText(bookmark.getName());
      final Location loc = MwmApplication.from(mIcon.getContext()).getLocationHelper().getSavedLocation();

      String distanceValue =
          loc == null
              ? ""
              : bookmark.getDistance(loc.getLatitude(), loc.getLongitude(), 0.0).toString(mDistance.getContext());
      String separator = "";
      if (!distanceValue.isEmpty() && !bookmark.getFeatureType().isEmpty())
        separator = " • ";
      String subtitleValue = distanceValue.concat(separator).concat(bookmark.getFeatureType());
      mDistance.setText(subtitleValue);
      UiUtils.hideIf(TextUtils.isEmpty(subtitleValue), mDistance);

      mIcon.setImageResource(bookmark.getIcon().getResId());
      Drawable circle =
          Graphics.drawCircleAndImage(bookmark.getIcon().argb(), R.dimen.track_circle_size,
                                      bookmark.getIcon().getResId(), R.dimen.bookmark_icon_size, mIcon.getContext());
      mIcon.setImageDrawable(circle);
    }
  }

  static class TrackViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final ShapeableImageView mIcon;
    @NonNull
    private final MaterialTextView mName;
    @NonNull
    private final MaterialTextView mDistance;
    private final ShapeableImageView mMoreButton;

    TrackViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__bookmark_color);
      mName = itemView.findViewById(R.id.tv__bookmark_name);
      mDistance = itemView.findViewById(R.id.tv__bookmark_distance);
      mMoreButton = itemView.findViewById(R.id.more);
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      final long trackId = sectionsDataSource.getTrackId(position);
      Track track = BookmarkManager.INSTANCE.getTrack(trackId);
      mName.setText(track.getName());
      mDistance.setText(new StringBuilder()
                            .append(mDistance.getContext().getString(R.string.length))
                            .append(" ")
                            .append(track.getLength().toString(mDistance.getContext()))
                            .toString());
      Drawable circle =
          Graphics.drawCircle(track.getColor(), R.dimen.track_circle_size, mIcon.getContext().getResources());
      mIcon.setImageDrawable(circle);
    }

    public void setMoreButtonClickListener(RecyclerClickListener listener)
    {
      mMoreButton.setOnClickListener(v -> listener.onItemClick(v, getBindingAdapterPosition()));
    }

    public void setTrackIconClickListener(IconClickListener listener)
    {
      mIcon.setOnClickListener(v -> listener.onItemClick((ShapeableImageView) v, getBindingAdapterPosition()));
    }
  }

  public static class SectionViewHolder extends BaseBookmarkHolder
  {
    @NonNull
    private final MaterialTextView mView;

    SectionViewHolder(@NonNull MaterialTextView itemView)
    {
      super(itemView);
      mView = itemView;
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      mView.setText(sectionsDataSource.getTitle(position.getSectionIndex(), mView.getResources()));
    }
  }

  static class DescriptionViewHolder extends BaseBookmarkHolder
  {
    static final float SPACING_MULTIPLE = 1.0f;
    static final float SPACING_ADD = 0.0f;
    @NonNull
    private final MaterialTextView mDescText;

    DescriptionViewHolder(@NonNull View itemView, @NonNull BookmarkCategory category)
    {
      super(itemView);
      mDescText = itemView.findViewById(R.id.text);
    }

    @Override
    void bind(@NonNull SectionPosition position, @NonNull BookmarkListAdapter.SectionsDataSource sectionsDataSource)
    {
      bindDescription(sectionsDataSource.getCategory());
    }

    private void bindDescription(@NonNull BookmarkCategory category)
    {
      String desc = TextUtils.isEmpty(category.getAnnotation()) ? category.getDescription() : category.getAnnotation();

      String formattedDesc = desc.replace("\n", "<br>");
      Spanned spannedDesc = Utils.fromHtml(formattedDesc);
      if (!TextUtils.isEmpty(spannedDesc)) {
        mDescText.setText(spannedDesc);
      }
      else {
        mDescText.setText(R.string.list_description_empty);
      }
    }
  }
}
