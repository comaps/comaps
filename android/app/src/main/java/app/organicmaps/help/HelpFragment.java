package app.organicmaps.help;

import android.content.res.Configuration;
import android.os.Bundle;
import android.text.TextUtils;
import android.text.util.Linkify;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.DateUtils;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.textview.MaterialTextView;

public class HelpFragment extends BaseMwmFragment implements View.OnClickListener
{
  private String mDonateUrl;
  private ActivityResultLauncher<SharingUtils.SharingIntent> shareLauncher;

  private void setupItem(@IdRes int id, boolean tint, @NonNull View frame)
  {
    final TextView view = frame.findViewById(id);
    view.setOnClickListener(this);
    if (tint)
      Graphics.tint(view);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mDonateUrl = Utils.getDonateUrl(requireContext());
    View root = inflater.inflate(R.layout.about, container, false);

    ((MaterialTextView) root.findViewById(R.id.version)).setText(BuildConfig.VERSION_NAME);

    final boolean isLandscape = getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE;

    final String dataVersion = DateUtils.getShortDateFormatter().format(Framework.getDataVersion());
    final MaterialTextView osmPresentationView = root.findViewById(R.id.osm_presentation);
    if (osmPresentationView != null)
    {
      osmPresentationView.setText(getString(R.string.osm_presentation, dataVersion));
      Linkify.addLinks(osmPresentationView, Linkify.WEB_URLS);
    }

    setupItem(R.id.faq, true, root);
    setupItem(R.id.news, true, root);
    setupItem(R.id.web, true, root);
    setupItem(R.id.code_repo, true, root);
    setupItem(R.id.mastodon, true, root);
    setupItem(R.id.matrix, true, root);
    setupItem(R.id.lemmy, true, root);
    setupItem(R.id.bluesky, true, root);
    setupItem(R.id.pixelfed, true, root);
    setupItem(R.id.openstreetmap, true, root);
    setupItem(R.id.email, true, root);
    setupItem(R.id.report, isLandscape, root);
    setupItem(R.id.copyright, false, root);

    final MaterialTextView supportUsView = root.findViewById(R.id.support_us);
    if (BuildConfig.FLAVOR.equals("google") && !TextUtils.isEmpty(mDonateUrl))
      supportUsView.setVisibility(View.GONE);
    else
      setupItem(R.id.support_us, true, root);

    final MaterialButton donateView = root.findViewById(R.id.donate);
    if (TextUtils.isEmpty(mDonateUrl))
      donateView.setVisibility(View.GONE);
    else
    {
      /*donateView.setCompoundDrawablesRelativeWithIntrinsicBounds(R.drawable.ic_donate, 0,
                R.drawable.ic_donate, 0);*/
      setupItem(R.id.donate, isLandscape, root);
    }

    if (BuildConfig.REVIEW_URL.isEmpty())
      root.findViewById(R.id.rate).setVisibility(View.GONE);
    else
      setupItem(R.id.rate, true, root);

    View termOfUseView = root.findViewById(R.id.term_of_use_link);
    View privacyPolicyView = root.findViewById(R.id.privacy_policy);
    termOfUseView.setOnClickListener(
        v -> Utils.openUrl(requireActivity(), getResources().getString(R.string.app_site_url) + "terms/"));
    privacyPolicyView.setOnClickListener(
        v -> Utils.openUrl(requireActivity(), getResources().getString(R.string.app_site_url) + "privacy/"));

    shareLauncher = SharingUtils.RegisterLauncher(this);

    ViewCompat.setOnApplyWindowInsetsListener(root, new ScrollableContentInsetsListener(root));

    return root;
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.web)
      Utils.openUrl(requireActivity(), getResources().getString(R.string.app_site_url));
    else if (id == R.id.news)
      Utils.openUrl(requireActivity(), "https://www.comaps.app/news/");
    else if (id == R.id.email)
      Utils.sendTo(requireContext(), BuildConfig.SUPPORT_MAIL, getString(R.string.project_name));
    else if (id == R.id.code_repo)
      Utils.openUrl(requireActivity(), Constants.Url.CODE_REPO);
    else if (id == R.id.mastodon)
      Utils.openUrl(requireActivity(), Constants.Url.MASTODON);
    else if (id == R.id.matrix)
      Utils.openUrl(requireActivity(), Constants.Url.MATRIX);
    else if (id == R.id.lemmy)
      Utils.openUrl(requireActivity(), Constants.Url.LEMMY);
    else if (id == R.id.bluesky)
      Utils.openUrl(requireActivity(), Constants.Url.BLUESKY);
    else if (id == R.id.pixelfed)
      Utils.openUrl(requireActivity(), Constants.Url.PIXELFED);
    else if (id == R.id.openstreetmap)
      Utils.openUrl(requireActivity(), getString(R.string.osm_wiki_about_url));
    else if (id == R.id.faq)
      ((HelpActivity) requireActivity()).stackFragment(FaqFragment.class, getString(R.string.faq), null);
    else if (id == R.id.report)
      Utils.sendBugReport(shareLauncher, requireActivity(), "", "");
    else if (id == R.id.support_us)
      Utils.openUrl(requireActivity(), getResources().getString(R.string.app_site_url) + "community/");
    else if (id == R.id.donate)
      Utils.openUrl(requireActivity(), mDonateUrl);
    else if (id == R.id.rate)
      Utils.openAppInMarket(requireActivity(), BuildConfig.REVIEW_URL);
    else if (id == R.id.copyright)
      ((HelpActivity) requireActivity()).stackFragment(CopyrightFragment.class, getString(R.string.copyright), null);
  }
}
