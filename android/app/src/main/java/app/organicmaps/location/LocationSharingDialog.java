package app.organicmaps.location;

import android.app.Dialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;

import app.organicmaps.R;
import app.organicmaps.util.SharingUtils;

/**
 * Dialog for starting/stopping live location sharing and managing the share URL.
 */
public class LocationSharingDialog extends DialogFragment
{
  private static final String TAG = LocationSharingDialog.class.getSimpleName();

  @Nullable
  private TextView mStatusText;
  @Nullable
  private TextView mShareUrlText;
  @Nullable
  private Button mStartStopButton;
  @Nullable
  private Button mCopyButton;
  @Nullable
  private Button mShareButton;

  private LocationSharingManager mManager;

  public static void show(@NonNull FragmentManager fragmentManager)
  {
    LocationSharingDialog dialog = new LocationSharingDialog();
    dialog.show(fragmentManager, TAG);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    mManager = LocationSharingManager.getInstance();

    AlertDialog.Builder builder = new AlertDialog.Builder(requireContext());
    View view = LayoutInflater.from(getContext()).inflate(R.layout.dialog_location_sharing, null);

    initViews(view);
    updateUI();

    builder.setView(view);
    builder.setTitle(R.string.location_sharing_title);
    builder.setNegativeButton(R.string.close, (dialog, which) -> dismiss());

    return builder.create();
  }

  private void initViews(@NonNull View root)
  {
    mStatusText = root.findViewById(R.id.status_text);
    mShareUrlText = root.findViewById(R.id.share_url_text);
    mStartStopButton = root.findViewById(R.id.start_stop_button);
    mCopyButton = root.findViewById(R.id.copy_button);
    mShareButton = root.findViewById(R.id.share_button);

    if (mStartStopButton != null)
    {
      mStartStopButton.setOnClickListener(v -> {
        if (mManager.isSharing())
          stopSharing();
        else
          startSharing();
      });
    }

    if (mCopyButton != null)
    {
      mCopyButton.setOnClickListener(v -> copyUrl());
    }

    if (mShareButton != null)
    {
      mShareButton.setOnClickListener(v -> shareUrl());
    }
  }

  private void updateUI()
  {
    boolean isSharing = mManager.isSharing();

    if (mStatusText != null)
    {
      mStatusText.setText(isSharing
          ? R.string.location_sharing_status_active
          : R.string.location_sharing_status_inactive);
    }

    if (mShareUrlText != null)
    {
      String url = mManager.getShareUrl();
      if (url != null && isSharing)
      {
        mShareUrlText.setText(url);
        mShareUrlText.setVisibility(View.VISIBLE);
      }
      else
      {
        mShareUrlText.setVisibility(View.GONE);
      }
    }

    if (mStartStopButton != null)
    {
      mStartStopButton.setText(isSharing
          ? R.string.location_sharing_stop
          : R.string.location_sharing_start);
    }

    // Show/hide copy and share buttons
    int visibility = isSharing ? View.VISIBLE : View.GONE;
    if (mCopyButton != null)
      mCopyButton.setVisibility(visibility);
    if (mShareButton != null)
      mShareButton.setVisibility(visibility);
  }

  private void startSharing()
  {
    String shareUrl = mManager.startSharing();

    if (shareUrl != null)
    {
      Toast.makeText(requireContext(),
          R.string.location_sharing_started,
          Toast.LENGTH_SHORT).show();

      updateUI();

      // Notify the activity
      if (getActivity() instanceof app.organicmaps.MwmActivity)
      {
        ((app.organicmaps.MwmActivity) getActivity()).onLocationSharingStateChanged(true);
      }

      // Auto-copy URL to clipboard
      copyUrlToClipboard(shareUrl);
    }
    else
    {
      Toast.makeText(requireContext(),
          R.string.location_sharing_failed_to_start,
          Toast.LENGTH_LONG).show();
    }
  }

  private void stopSharing()
  {
    mManager.stopSharing();

    Toast.makeText(requireContext(),
        R.string.location_sharing_stopped,
        Toast.LENGTH_SHORT).show();

    updateUI();

    // Notify the activity
    if (getActivity() instanceof app.organicmaps.MwmActivity)
    {
      ((app.organicmaps.MwmActivity) getActivity()).onLocationSharingStateChanged(false);
    }
  }

  private void copyUrl()
  {
    String url = mManager.getShareUrl();
    if (url != null)
    {
      copyUrlToClipboard(url);
    }
  }

  private void copyUrlToClipboard(@NonNull String url)
  {
    ClipboardManager clipboard = (ClipboardManager)
        requireContext().getSystemService(Context.CLIPBOARD_SERVICE);

    if (clipboard != null)
    {
      ClipData clip = ClipData.newPlainText("Location Share URL", url);
      clipboard.setPrimaryClip(clip);

      Toast.makeText(requireContext(),
          R.string.location_sharing_url_copied,
          Toast.LENGTH_SHORT).show();
    }
  }

  private void shareUrl()
  {
    String url = mManager.getShareUrl();
    if (url == null)
      return;

    Intent shareIntent = new Intent(Intent.ACTION_SEND);
    shareIntent.setType("text/plain");
    shareIntent.putExtra(Intent.EXTRA_TEXT, getString(R.string.location_sharing_share_message, url));
    startActivity(Intent.createChooser(shareIntent, getString(R.string.location_sharing_share_url)));
  }
}
