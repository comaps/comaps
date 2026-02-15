package app.organicmaps.downloader;

import android.location.Location;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.ConnectionState;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import app.organicmaps.widget.WheelProgressView;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.textview.MaterialTextView;
import java.util.List;

public class OnmapDownloader implements MwmActivity.LeftAnimationTrackListener
{
  private static boolean sAutodownloadLocked;

  private static final int HIDE_THRESHOLD = 2;
  // Default bundles (e.g., world/coasts). Used to approximate “user-downloaded” count.
  private static final int DEFAULT_MAP_BASELINE = 2;

  private final MwmActivity mActivity;
  private final View mFrame;
  private final MaterialTextView mParent;
  private final MaterialTextView mTitle;
  private final MaterialTextView mSize;
  private final WheelProgressView mProgress;
  private final MaterialButton mButton;
  private final View mOfflineExplanation;

  private int mStorageSubscriptionSlot;

  @Nullable
  private CountryItem mCurrentCountry;

  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      if (mCurrentCountry == null) {
        updateOfflineExplanationVisibility();
        return;
      }

      for (MapManager.StorageCallbackData item : data)
      {
        if (!item.isLeafNode)
          continue;

        if (item.newStatus == CountryItem.STATUS_FAILED)
          MapManagerHelper.showError(mActivity, item, null);

        if (mCurrentCountry.id.equals(item.countryId))
        {
          mCurrentCountry.update();
          updateProgressState(false);
          updateOfflineExplanationVisibility();
          return;
        }
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mCurrentCountry != null && mCurrentCountry.id.equals(countryId))
      {
        mCurrentCountry.update();
        updateProgressState(false);
      }
    }
  };

  private final MapManager.CurrentCountryChangedListener mCountryChangedListener =
      new MapManager.CurrentCountryChangedListener() {
        @Override
        public void onCurrentCountryChanged(String countryId)
        {
          mCurrentCountry = (TextUtils.isEmpty(countryId) ? null : CountryItem.fill(countryId));
          updateState(true);
        }
      };

  public void updateState(boolean shouldAutoDownload)
  {
    updateStateInternal(shouldAutoDownload);
  }

  private static boolean isMapDownloading(@Nullable CountryItem country)
  {
    if (country == null)
      return false;

    boolean enqueued = country.status == CountryItem.STATUS_ENQUEUED;
    boolean progress = country.status == CountryItem.STATUS_PROGRESS;
    boolean applying = country.status == CountryItem.STATUS_APPLYING;
    return enqueued || progress || applying;
  }

  private void updateOfflineExplanationVisibility() {
    if (mOfflineExplanation == null) return;
    // hide once threshold reached; safe to call repeatedly.
    app.organicmaps.util.UiUtils.showIf(MapManager.nativeGetDownloadedCount() < (DEFAULT_MAP_BASELINE + HIDE_THRESHOLD), mOfflineExplanation);
  }

  private void updateProgressState(boolean shouldAutoDownload)
  {
    updateStateInternal(shouldAutoDownload);
  }

  private void updateStateInternal(boolean shouldAutoDownload)
  {
    updateOfflineExplanationVisibility();

    boolean showFrame =
        (mCurrentCountry != null && !mCurrentCountry.present && !RoutingController.get().isNavigating());
    if (showFrame)
    {
      boolean enqueued = (mCurrentCountry.status == CountryItem.STATUS_ENQUEUED);
      boolean progress = (mCurrentCountry.status == CountryItem.STATUS_PROGRESS
                          || mCurrentCountry.status == CountryItem.STATUS_APPLYING);
      boolean failed = (mCurrentCountry.status == CountryItem.STATUS_FAILED);

      showFrame = (enqueued || progress || failed || mCurrentCountry.status == CountryItem.STATUS_DOWNLOADABLE);

      if (showFrame)
      {
        boolean hasParent = !CountryItem.isRoot(mCurrentCountry.topmostParentId);

        UiUtils.showIf(progress || enqueued, mProgress);
        UiUtils.showIf(!progress && !enqueued, mButton);
        UiUtils.showIf(hasParent, mParent);

        if (hasParent)
          mParent.setText(mCurrentCountry.topmostParentName);

        mTitle.setText(mCurrentCountry.name);

        String sizeText;

        if (progress)
        {
          int roundedProgress = Math.round(mCurrentCountry.progress);
          mProgress.setPending(false);
          mProgress.setProgress(roundedProgress);
          sizeText = mActivity.getString(R.string.downloader_downloading) + " "
                   + StringUtils.formatPercent(roundedProgress / 100.0);
        }
        else
        {
          if (enqueued)
          {
            sizeText = mActivity.getString(R.string.downloader_queued);
            mProgress.setPending(true);
          }
          else
          {
            sizeText = StringUtils.getFileSizeString(mActivity.getApplicationContext(), mCurrentCountry.totalSize);

            if (shouldAutoDownload && Config.isAutodownloadEnabled() && !sAutodownloadLocked && !failed
                && ConnectionState.INSTANCE.isWifiConnected())
            {
              Location loc = MwmApplication.from(mActivity).getLocationHelper().getSavedLocation();
              if (loc != null)
              {
                String country = MapManager.nativeFindCountry(loc.getLatitude(), loc.getLongitude());
                if (TextUtils.equals(mCurrentCountry.id, country)
                    && MapManager.nativeHasSpaceToDownloadCountry(country))
                {
                  MapManagerHelper.startDownload(mCurrentCountry.id);
                }
              }
            }

            mButton.setText(failed ? R.string.downloader_retry : R.string.download);
          }
        }

        mSize.setText(sizeText);
      }
    }

    UiUtils.showIf(showFrame, mFrame);
  }

  public OnmapDownloader(MwmActivity activity)
  {
    mActivity = activity;
    mFrame = activity.findViewById(R.id.onmap_downloader);
    mParent = mFrame.findViewById(R.id.downloader_parent);
    mTitle = mFrame.findViewById(R.id.downloader_title);
    mSize = mFrame.findViewById(R.id.downloader_size);

    View controls = mFrame.findViewById(R.id.downloader_controls_frame);
    mProgress = controls.findViewById(R.id.wheel_downloader_progress);
    mButton = controls.findViewById(R.id.downloader_button);

    mOfflineExplanation = mFrame.findViewById(R.id.offline_explanation);
    updateOfflineExplanationVisibility();

    mProgress.setOnClickListener(v -> {
      if (mCurrentCountry == null)
        return;

      MapManager.nativeCancel(mCurrentCountry.id);
      setAutodownloadLocked(true);
    });
    mButton.setOnClickListener(
        v -> MapManagerHelper.warnOn3g(mActivity, mCurrentCountry == null ? null : mCurrentCountry.id, () -> {
          if (mCurrentCountry == null)
            return;

          boolean retry = (mCurrentCountry.status == CountryItem.STATUS_FAILED);
          if (retry)
          {
            MapManagerHelper.retryDownload(mCurrentCountry.id);
          }
          else
          {
            MapManagerHelper.startDownload(mCurrentCountry.id);
            mActivity.requestPostNotificationsPermission();
          }
        }));

    ViewCompat.setOnApplyWindowInsetsListener(mFrame, PaddingInsetsListener.allSides());
  }

  @Override
  public void onTrackStarted(boolean collapsed)
  {}

  @Override
  public void onTrackFinished(boolean collapsed)
  {}

  @Override
  public void onTrackLeftAnimation(float offset)
  {
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mFrame.getLayoutParams();
    lp.leftMargin = (int) offset;
    mFrame.setLayoutParams(lp);
  }

  public void onPause()
  {
    if (mStorageSubscriptionSlot > 0)
    {
      MapManager.nativeUnsubscribe(mStorageSubscriptionSlot);
      mStorageSubscriptionSlot = 0;

      MapManager.nativeUnsubscribeOnCountryChanged();
    }
  }

  public void onResume()
  {
    updateOfflineExplanationVisibility();
    if (mStorageSubscriptionSlot == 0)
    {
      mStorageSubscriptionSlot = MapManager.nativeSubscribe(mStorageCallback);

      MapManager.nativeSubscribeOnCountryChanged(mCountryChangedListener);
    }
  }

  public static void setAutodownloadLocked(boolean locked)
  {
    sAutodownloadLocked = locked;
  }
}
