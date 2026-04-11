package app.organicmaps.downloader;

import android.util.SparseIntArray;
import android.view.View;
import androidx.annotation.DrawableRes;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.WheelProgressView;
import com.google.android.material.imageview.ShapeableImageView;

public class DownloaderStatusIcon
{
  private final View mFrame;
  protected final ShapeableImageView mIcon;
  private final WheelProgressView mProgress;

  private static final SparseIntArray sIconsCache = new SparseIntArray();

  public DownloaderStatusIcon(View frame)
  {
    mFrame = frame;
    mIcon = mFrame.findViewById(R.id.downloader_status);
    mProgress = mFrame.findViewById(R.id.downloader_progress_wheel);
  }

  public DownloaderStatusIcon setOnIconClickListener(View.OnClickListener listener)
  {
    mIcon.setOnClickListener(listener);
    return this;
  }

  public DownloaderStatusIcon setOnCancelClickListener(View.OnClickListener listener)
  {
    mProgress.setOnClickListener(listener);
    return this;
  }

  protected @DrawableRes int selectIcon(CountryItem country)
  {
    return switch (country.status)
    {
      case CountryItem.STATUS_DONE -> R.drawable.downloader_done;
      case CountryItem.STATUS_DOWNLOADABLE, CountryItem.STATUS_PARTLY -> R.drawable.downloader_download;
      case CountryItem.STATUS_FAILED -> R.drawable.downloader_failed;
      case CountryItem.STATUS_UPDATABLE -> R.drawable.downloader_update;
      default -> throw new IllegalArgumentException("Inappropriate item status: " + country.status);
    };
  }

  protected void updateIcon(CountryItem country)
  {
    mIcon.setImageResource(selectIcon(country));
  }

  public void update(CountryItem country)
  {
    boolean pending = (country.status == CountryItem.STATUS_ENQUEUED);
    boolean inProgress =
        (country.status == CountryItem.STATUS_PROGRESS || country.status == CountryItem.STATUS_APPLYING || pending);

    UiUtils.showIf(inProgress, mProgress);
    UiUtils.showIf(!inProgress, mIcon);
    mProgress.setPending(pending);

    if (inProgress)
    {
      if (!pending)
        mProgress.setProgress(Math.round(country.progress));
      return;
    }

    updateIcon(country);
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
  }

  public static void clearCache()
  {
    sIconsCache.clear();
  }
}
