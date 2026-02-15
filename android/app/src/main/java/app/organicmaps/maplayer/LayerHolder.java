package app.organicmaps.maplayer;

import android.view.View;
import android.widget.ImageView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.textview.MaterialTextView;

import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;

class LayerHolder extends RecyclerView.ViewHolder
{
  @NonNull
  final ImageView mButton;
  @NonNull
  final MaterialTextView mTitle;
  @NonNull
  final View mNewMarker;
  @Nullable
  LayerBottomSheetItem mItem;
  @Nullable
  OnItemClickListener<LayerBottomSheetItem> mListener;

  LayerHolder(@NonNull View root)
  {
    super(root);
    mTitle = root.findViewById(R.id.name);
    mNewMarker = root.findViewById(R.id.marker);
    mButton = root.findViewById(R.id.btn);
    mButton.setOnClickListener(this::onItemClicked);
  }

  public void onItemClicked(@NonNull View v)
  {
    if (mListener != null && mItem != null)
      mListener.onItemClick(v, mItem);
  }
}
