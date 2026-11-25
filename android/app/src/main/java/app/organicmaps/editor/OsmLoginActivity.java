package app.organicmaps.editor;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmFragmentActivity;

public class OsmLoginActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OAUTH2CODE = "oauth2code";
  public static final String EXTRA_OAUTH2STATE = "oauth2state";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return OsmLoginFragment.class;
  }

  public static void OAuth2Callback(@NonNull Activity activity, String oauth2code, String oauth2state)
  {
    final Intent i = new Intent(activity, OsmLoginActivity.class);
    Bundle args = new Bundle();
    args.putString(EXTRA_OAUTH2CODE, oauth2code);
    args.putString(EXTRA_OAUTH2STATE, oauth2state);
    args.putBoolean(ProfileActivity.EXTRA_REDIRECT_TO_PROFILE, true);
    i.putExtras(args);
    activity.startActivity(i);
  }
}
