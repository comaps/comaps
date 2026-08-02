package app.organicmaps.widget.placepage;

import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static app.organicmaps.util.Utils.openUrl;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.os.Build;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import androidx.fragment.app.FragmentManager;

import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.FeatureId;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.editor.ReviewEditor;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.MobileDataDialogFragment;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class AddReviewController
{
  private static final ExecutorService mUrlLookupExecutor = Executors.newCachedThreadPool();

  @NonNull
  private final View mView;
  @NonNull
  private final FragmentManager mFragmentManager;

  public AddReviewController(@NonNull View mView, @NonNull FragmentManager mFragmentManager)
  {
    this.mView = mView;
    this.mFragmentManager = mFragmentManager;
  }

  public void updateView(@Nullable String reviewEditorAppName, @NonNull FeatureId featureId)
  {
    mView.setVisibility(GONE);
    if (reviewEditorAppName != null)
    {
      TextView addReviewText = mView.findViewById(R.id.tv__add_review);
      addReviewText.setText(mView.getResources().getString(R.string.add_review, reviewEditorAppName));
      mView.setOnClickListener((v) -> onClick(featureId));
      mView.setVisibility(VISIBLE);
    }
  }

  private void onClick(@NonNull FeatureId featureId)
  {
    Context ctx = mView.getContext();
    if (ctx == null)
      return;
    ConnectivityManager connectivityManager = ((ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE));
    if (connectivityManager != null)
    {
      if (!checkInternetConnectivity(connectivityManager, ctx))
        return;
    }
    NetworkPolicy.DialogPresenter networkPolicyDialog = new MobileDataDialogFragment.Presenter();
    NetworkPolicy.checkNetworkPolicy(networkPolicyDialog, mFragmentManager, policy -> {
      if (!policy.canUseNetwork())
      {
        Utils.showSnackbar(ctx, mView, R.string.internet_unavailable);
        return;
      }
      mUrlLookupExecutor.execute(() -> resolveAndOpenEditorUrl(featureId));
    });
  }

  /**
   * Checks if Internet connectivity is available and notifies the user if it is not.
   *
   * @return <code>true</code> if Internet connectivity is potentially available
   */
  private boolean checkInternetConnectivity(@NonNull ConnectivityManager connectivityManager, @NonNull Context ctx)
  {
    // For very old SDK versions we don't bother to check - different API calls are required and
    // it is a minor convenience feature only.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
      return true;
    boolean internetPotentiallyAvailable = true;
    Network network = connectivityManager.getActiveNetwork();
    if (network == null)
    {
      internetPotentiallyAvailable = false;
    }
    else
    {
      NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(network);
      if (capabilities != null && !capabilities.hasCapability(NET_CAPABILITY_INTERNET))
        internetPotentiallyAvailable = false;
    }
    if (!internetPotentiallyAvailable)
      Utils.showSnackbar(ctx, mView, R.string.internet_unavailable);
    return internetPotentiallyAvailable;
  }

  @WorkerThread
  private void resolveAndOpenEditorUrl(@NonNull FeatureId featureId)
  {
    String url = ReviewEditor.nativeGetReviewEditorUrl(featureId);
    // This is invoked asynchronously, on a worker thread, so we need to be extra careful about the state of the UI - it might have
    // changed between the user initiating the action and the URL being resolved.
    mView.post(() -> {
      Context ctx = mView.getContext();
      if (ctx == null)
        return;
      if (url == null)
      {
        // user might have closed the page before the URL resolution finished. If so, don't attempt to show the snackbar
        if (mView.getParent() != null)
          Utils.showSnackbar(ctx, mView, OsmOAuth.isDev() ? R.string.add_review_url_resolution_failed_dev : R.string.add_review_url_resolution_failed);
      }
      else
        openUrl(ctx, url);
    });
  }
}
