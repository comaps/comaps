package app.organicmaps.widget.placepage.sections;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.textview.MaterialTextView;
import java.util.ArrayList;
import java.util.List;

public class PlacePageLinksFragment extends Fragment implements Observer<MapObject>
{
  private static final String TAG = PlacePageLinksFragment.class.getSimpleName();

  private View mFrame;
  private View mFacebookPage;
  private MaterialTextView mTvFacebookPage;
  private View mInstagramPage;
  private MaterialTextView mTvInstagramPage;
  private View mTwitterPage;
  private MaterialTextView mTvTwitterPage;
  private View mFediversePage;
  private MaterialTextView mTvFediversePage;
  private View mBlueskyPage;
  private MaterialTextView mTvBlueskyPage;
  private View mVkPage;
  private MaterialTextView mTvVkPage;
  private View mLinePage;
  private MaterialTextView mTvLinePage;

  private View mWebsite;
  private MaterialTextView mTvWebsite;
  private View mWebsiteMenu;
  private MaterialTextView mTvWebsiteMenuSubsite;
  private View mEmail;
  private MaterialTextView mTvEmail;
  private View mWikimedia;
  private MaterialTextView mTvWikimedia;

  private View mPanoramax;
  private MaterialTextView mTvPanoramax;

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

  @NonNull
  private String getLink(@NonNull Metadata.MetadataType type)
  {
    return switch (type)
    {
      case FMD_WEBSITE -> mMapObject.getWebsiteUrl(false /* strip */, Metadata.MetadataType.FMD_WEBSITE);
      case FMD_WEBSITE_MENU -> mMapObject.getWebsiteUrl(false /* strip */, Metadata.MetadataType.FMD_WEBSITE_MENU);
      case FMD_CONTACT_FACEBOOK, FMD_CONTACT_INSTAGRAM, FMD_CONTACT_TWITTER, FMD_CONTACT_FEDIVERSE, FMD_CONTACT_BLUESKY,
          FMD_CONTACT_VK, FMD_CONTACT_LINE, FMD_PANORAMAX ->
      {
        if (TextUtils.isEmpty(mMapObject.getMetadata(type)))
          yield "";
        yield Framework.nativeGetPoiContactUrl(type.toInt());
      }
      default -> mMapObject.getMetadata(type);
    };
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_links_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mFrame = view;

    mWebsite = mFrame.findViewById(R.id.ll__place_website);
    mTvWebsite = mFrame.findViewById(R.id.tv__place_website);
    mWebsite.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_WEBSITE));
    mWebsite.setOnLongClickListener((v) -> copyUrl(mWebsite, Metadata.MetadataType.FMD_WEBSITE));

    mWebsiteMenu = mFrame.findViewById(R.id.ll__place_website_menu);
    mTvWebsiteMenuSubsite = mFrame.findViewById(R.id.tv__place_website_menu_subtitle);
    mWebsiteMenu.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_WEBSITE_MENU));
    mWebsiteMenu.setOnLongClickListener((v) -> copyUrl(mWebsiteMenu, Metadata.MetadataType.FMD_WEBSITE_MENU));

    mEmail = mFrame.findViewById(R.id.ll__place_email);
    mTvEmail = mFrame.findViewById(R.id.tv__place_email);
    mEmail.setOnClickListener(v -> {
      final String email = mMapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL);
      if (!TextUtils.isEmpty(email))
        Utils.sendTo(requireContext(), email);
    });
    mEmail.setOnLongClickListener((v) -> copyUrl(mEmail, Metadata.MetadataType.FMD_EMAIL));

    mWikimedia = mFrame.findViewById(R.id.ll__place_wikimedia);
    mTvWikimedia = mFrame.findViewById(R.id.tv__place_wikimedia);
    mWikimedia.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));
    mWikimedia.setOnLongClickListener((v) -> copyUrl(mWikimedia, Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));

    mFacebookPage = mFrame.findViewById(R.id.ll__place_facebook);
    mTvFacebookPage = mFrame.findViewById(R.id.tv__place_facebook_page);
    mFacebookPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_FACEBOOK));
    mFacebookPage.setOnLongClickListener((v) -> copyUrl(mFacebookPage, Metadata.MetadataType.FMD_CONTACT_FACEBOOK));

    mInstagramPage = mFrame.findViewById(R.id.ll__place_instagram);
    mTvInstagramPage = mFrame.findViewById(R.id.tv__place_instagram_page);
    mInstagramPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM));
    mInstagramPage.setOnLongClickListener((v) -> copyUrl(mInstagramPage, Metadata.MetadataType.FMD_CONTACT_INSTAGRAM));

    mFediversePage = mFrame.findViewById(R.id.ll__place_fediverse);
    mTvFediversePage = mFrame.findViewById(R.id.tv__place_fediverse_page);
    mFediversePage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_FEDIVERSE));
    mFediversePage.setOnLongClickListener((v) -> copyUrl(mFediversePage, Metadata.MetadataType.FMD_CONTACT_FEDIVERSE));

    mBlueskyPage = mFrame.findViewById(R.id.ll__place_bluesky);
    mTvBlueskyPage = mFrame.findViewById(R.id.tv__place_bluesky_page);
    mBlueskyPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_BLUESKY));
    mBlueskyPage.setOnLongClickListener((v) -> copyUrl(mBlueskyPage, Metadata.MetadataType.FMD_CONTACT_BLUESKY));

    mTwitterPage = mFrame.findViewById(R.id.ll__place_twitter);
    mTvTwitterPage = mFrame.findViewById(R.id.tv__place_twitter_page);
    mTwitterPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_TWITTER));
    mTwitterPage.setOnLongClickListener((v) -> copyUrl(mTwitterPage, Metadata.MetadataType.FMD_CONTACT_TWITTER));

    mVkPage = mFrame.findViewById(R.id.ll__place_vk);
    mTvVkPage = mFrame.findViewById(R.id.tv__place_vk_page);
    mVkPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_VK));
    mVkPage.setOnLongClickListener((v) -> copyUrl(mVkPage, Metadata.MetadataType.FMD_CONTACT_VK));

    mLinePage = mFrame.findViewById(R.id.ll__place_line);
    mTvLinePage = mFrame.findViewById(R.id.tv__place_line_page);
    mLinePage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_LINE));
    mLinePage.setOnLongClickListener((v) -> copyUrl(mLinePage, Metadata.MetadataType.FMD_CONTACT_LINE));

    mPanoramax = mFrame.findViewById(R.id.ll__place_panoramax);
    mTvPanoramax = mFrame.findViewById(R.id.tv__place_panoramax);
    mPanoramax.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_PANORAMAX));
    mPanoramax.setOnLongClickListener((v) -> copyUrl(mPanoramax, Metadata.MetadataType.FMD_PANORAMAX));
  }

  private void openUrl(Metadata.MetadataType type)
  {
    final String url = getLink(type);
    if (!TextUtils.isEmpty(url))
      Utils.openUrl(requireContext(), url);
  }

  private boolean copyUrl(View view, Metadata.MetadataType type)
  {
    final String url = getLink(type);
    if (TextUtils.isEmpty(url))
      return false;
    final List<String> items = new ArrayList<>();
    items.add(url);

    final String title = switch (type)
    {
      case FMD_WEBSITE -> mMapObject.getWebsiteUrl(false /* strip */, Metadata.MetadataType.FMD_WEBSITE);
      case FMD_WEBSITE_MENU -> mMapObject.getWebsiteUrl(false /* strip */, Metadata.MetadataType.FMD_WEBSITE_MENU);
      case FMD_PANORAMAX -> null; // Don't add raw ID to list, as it's useless for users.
      default -> mMapObject.getMetadata(type);
    };
        // Add user names for social media if available
        if (!TextUtils.isEmpty(title) && !title.equals(url) && !title.contains("/")) items.add(title);

    if (items.size() == 1)
      PlacePageUtils.copyToClipboard(requireContext(), mFrame, items.get(0));
    else
      PlacePageUtils.showCopyPopup(requireContext(), view, items);
    return true;
  }

  private void refreshLinks()
  {
    refreshMetadataOrHide(mMapObject.getWebsiteUrl(true /* strip */, Metadata.MetadataType.FMD_WEBSITE), mWebsite,
                          mTvWebsite);
    refreshMetadataOrHide(mMapObject.getWebsiteUrl(true /* strip */, Metadata.MetadataType.FMD_WEBSITE_MENU),
                          mWebsiteMenu, mTvWebsiteMenuSubsite);

    String wikimedia_commons = mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS);
    String wikimedia_commons_text =
        TextUtils.isEmpty(wikimedia_commons) ? "" : getResources().getString(R.string.wikimedia_commons);
    refreshMetadataOrHide(wikimedia_commons_text, mWikimedia, mTvWikimedia);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);

    final String facebook = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
    refreshMetadataOrHide(facebook, mFacebookPage, mTvFacebookPage);

    final String instagram = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
    refreshMetadataOrHide(instagram, mInstagramPage, mTvInstagramPage);

    final String fediverse = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FEDIVERSE);
    refreshMetadataOrHide(fediverse, mFediversePage, mTvFediversePage);

    final String bluesky = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_BLUESKY);
    refreshMetadataOrHide(bluesky, mBlueskyPage, mTvBlueskyPage);

    final String twitter = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
    refreshMetadataOrHide(twitter, mTwitterPage, mTvTwitterPage);

    final String vk = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
    refreshMetadataOrHide(vk, mVkPage, mTvVkPage);

    final String line = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
    refreshMetadataOrHide(line, mLinePage, mTvLinePage);

    final String panoramax = mMapObject.getMetadata(Metadata.MetadataType.FMD_PANORAMAX);
    final String panoramaxTitle = TextUtils.isEmpty(panoramax) ? "" : getResources().getString(R.string.panoramax);
    refreshMetadataOrHide(panoramaxTitle, mPanoramax, mTvPanoramax);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject != null)
    {
      mMapObject = mapObject;
      refreshLinks();
    }
  }
}
