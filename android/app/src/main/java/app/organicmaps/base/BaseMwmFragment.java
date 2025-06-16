package app.organicmaps.base;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.util.Utils;

public class BaseMwmFragment extends Fragment implements OnBackPressListener
{
  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    Utils.detachFragmentIfCoreNotInitialized(context, this);
  }

  @Override
  public boolean onBackPressed()
  {
    return false;
  }
}
