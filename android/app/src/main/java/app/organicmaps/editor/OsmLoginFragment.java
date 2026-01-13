package app.organicmaps.editor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ScrollView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class OsmLoginFragment extends BaseMwmToolbarFragment
{
  private MaterialButton mLoginButton;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_osm_login, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.login);
    mLoginButton = view.findViewById(R.id.login);
    MaterialButton registerButton = view.findViewById(R.id.register);
    registerButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_REGISTER));
    mLoginButton.setOnClickListener((v) -> loginWithBrowser());

    String code = readOAuth2CodeFromArguments();
    if (code != null && !code.isEmpty())
      continueOAuth2Flow(code);

    ScrollView scrollView = view.findViewById(R.id.scrollView);
    ViewCompat.setOnApplyWindowInsetsListener(scrollView, new ScrollableContentInsetsListener(scrollView));
  }

  private String readOAuth2CodeFromArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return null;

    return arguments.getString(OsmLoginActivity.EXTRA_OAUTH2CODE);
  }

  private void loginWithBrowser()
  {
    Utils.openUri(requireContext(), Uri.parse(OsmOAuth.nativeGetOAuth2Url()), R.string.browser_not_available);
  }

  private void processAuth(String oauthToken, String username)
  {
    if (!isAdded())
      return;

    mLoginButton.setEnabled(true);
    mLoginButton.setText(R.string.login_osm);
    if (oauthToken == null)
      onAuthFail();
    else
      onAuthSuccess(oauthToken, username);
  }

  private void onAuthFail()
  {
    new MaterialAlertDialogBuilder(requireActivity())
        .setTitle(R.string.editor_login_error_dialog)
        .setPositiveButton(R.string.ok, null)
        .show();
  }

  private void onAuthSuccess(String oauthToken, String username)
  {
    OsmOAuth.setAuthorization(oauthToken, username);
    final Bundle extras = requireActivity().getIntent().getExtras();
    if (extras != null && extras.getBoolean("redirectToProfile", false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }

  // This method is called by MwmActivity & UrlProcessor when "cm://oauth2/osm/callback?code=XXX" is handled
  private void continueOAuth2Flow(String oauth2code)
  {
    if (!isAdded())
      return;

    if (oauth2code == null || oauth2code.isEmpty())
      onAuthFail();
    else
    {
      ThreadPool.getWorker().execute(() -> {
        // Finish OAuth2 auth flow and get username for UI.
        final String oauthToken = OsmOAuth.nativeAuthWithOAuth2Code(oauth2code);
        final String username = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
        UiThread.run(() -> processAuth(oauthToken, username));
      });
    }
  }
}
