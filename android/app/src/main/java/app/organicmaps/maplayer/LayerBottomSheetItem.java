package app.organicmaps.maplayer;

import android.content.Context;
import android.view.View;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import app.organicmaps.R;
import app.organicmaps.adapter.OnItemClickListener;
import app.organicmaps.sdk.maplayer.Mode;

public class LayerBottomSheetItem
{
  @DrawableRes
  private final int mDrawableResId;
  @StringRes
  private final int mTitleResId;
  @NonNull
  private final Mode mMode;
  @NonNull
  private final OnItemClickListener<LayerBottomSheetItem> mItemClickListener;

  LayerBottomSheetItem(@DrawableRes int drawableResId, @StringRes int titleResId, @NonNull Mode mode,
                       @NonNull OnItemClickListener<LayerBottomSheetItem> itemClickListener)
  {
    mDrawableResId = drawableResId;
    mTitleResId = titleResId;
    mMode = mode;
    mItemClickListener = itemClickListener;
  }

  public static LayerBottomSheetItem create(@NonNull Context mContext, Mode mode,
                                            @NonNull OnItemClickListener<LayerBottomSheetItem> layerItemClickListener)
  {
    @DrawableRes
    int drawableResId = 0;
    @StringRes
    int buttonTextResource = switch (mode) {
        case OUTDOORS -> {
            drawableResId = R.drawable.ic_layers_outdoors;
            yield R.string.button_layer_outdoor;
        }
        case SUBWAY -> {
            drawableResId = R.drawable.ic_layers_subway;
            yield R.string.subway;
        }
        case ISOLINES -> {
            drawableResId = R.drawable.ic_layers_isoline;
            yield R.string.button_layer_isolines;
        }
        case TRAFFIC -> {
            drawableResId = R.drawable.ic_layers_traffic;
            yield R.string.button_layer_traffic;
        }
    };
      return new LayerBottomSheetItem(drawableResId, buttonTextResource, mode, layerItemClickListener);
  }

  @NonNull
  public Mode getMode()
  {
    return mMode;
  }

  @DrawableRes
  public int getDrawable()
  {
    return mDrawableResId;
  }

  @StringRes
  public int getTitle()
  {
    return mTitleResId;
  }

  public void onClick(@NonNull View v, @NonNull LayerBottomSheetItem item)
  {
    mItemClickListener.onItemClick(v, item);
  }
}
