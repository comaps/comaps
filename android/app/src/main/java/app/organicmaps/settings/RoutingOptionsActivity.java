package app.organicmaps.settings;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.settings.RoutingOptionsFragment;

public class RoutingOptionsActivity extends BaseMwmFragmentActivity
{
  public static void start(@NonNull Activity activity, ActivityResultLauncher<Intent> startRoutingOptionsForResult)
  {
    Intent intent = new Intent(activity, RoutingOptionsActivity.class);
    startRoutingOptionsForResult.launch(intent);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return RoutingOptionsFragment.class;
  }
}
