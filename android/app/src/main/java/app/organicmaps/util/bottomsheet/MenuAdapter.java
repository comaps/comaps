package app.organicmaps.util.bottomsheet;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.location.TrackRecorder;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textview.MaterialTextView;
import java.util.ArrayList;

public class MenuAdapter extends RecyclerView.Adapter<MenuAdapter.ViewHolder>
{
  private final ArrayList<MenuBottomSheetItem> dataSet;
  @Nullable
  private final MenuBottomSheetItem.OnClickListener onClickListener;

  public MenuAdapter(ArrayList<MenuBottomSheetItem> dataSet,
                     @Nullable MenuBottomSheetItem.OnClickListener onClickListener)
  {
    this.dataSet = dataSet;
    this.onClickListener = onClickListener;
  }

  private void onMenuItemClick(MenuBottomSheetItem item)
  {
    if (onClickListener != null)
      onClickListener.onClick();
    item.onClickListener.onClick();
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int viewType)
  {
    View view = LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.bottom_sheet_menu_item, viewGroup, false);
    return new ViewHolder(view);
  }

  @Override
  public void onBindViewHolder(ViewHolder viewHolder, final int position)
  {
    final MenuBottomSheetItem item = dataSet.get(position);
    final ShapeableImageView iv = viewHolder.getIconImageView();
    iv.setImageResource(item.iconRes);
    viewHolder.getContainer().setOnClickListener((v) -> onMenuItemClick(item));
    viewHolder.getTitleTextView().setText(item.titleRes);
    MaterialTextView badge = viewHolder.getBadgeTextView();
    if (item.badgeCount > 0)
    {
      badge.setText(String.valueOf(item.badgeCount));
      badge.setVisibility(View.VISIBLE);
    }
    else
    {
      badge.setVisibility(View.GONE);
    }

    if (item.iconRes == R.drawable.ic_track_recording_off && TrackRecorder.nativeIsTrackRecordingEnabled())
    {
      iv.setImageResource(R.drawable.ic_track_recording_on);
      iv.setImageTintMode(null);
      viewHolder.getTitleTextView().setText(R.string.stop_track_recording);
    }
  }

  @Override
  public int getItemCount()
  {
    return dataSet.size();
  }

  public static class ViewHolder extends RecyclerView.ViewHolder
  {
    private final LinearLayout container;
    private final ShapeableImageView iconImageView;
    private final MaterialTextView titleTextView;
    private final MaterialTextView badgeTextView;

    public ViewHolder(View view)
    {
      super(view);
      container = view.findViewById(R.id.bottom_sheet_menu_item);
      iconImageView = view.findViewById(R.id.bottom_sheet_menu_item_icon);
      titleTextView = view.findViewById(R.id.bottom_sheet_menu_item_text);
      badgeTextView = view.findViewById(R.id.bottom_sheet_menu_item_badge);
    }

    public ShapeableImageView getIconImageView()
    {
      return iconImageView;
    }

    public MaterialTextView getTitleTextView()
    {
      return titleTextView;
    }

    public MaterialTextView getBadgeTextView()
    {
      return badgeTextView;
    }

    public LinearLayout getContainer()
    {
      return container;
    }
  }
}
