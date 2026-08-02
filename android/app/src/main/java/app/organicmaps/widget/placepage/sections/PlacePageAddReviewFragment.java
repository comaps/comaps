package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.widget.placepage.AddReviewController;
import app.organicmaps.widget.placepage.PlacePageViewModel;

public final class PlacePageAddReviewFragment extends Fragment implements Observer<MapObject>
{
  private PlacePageViewModel mViewModel;
  private AddReviewController mController;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_add_review, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mController = new AddReviewController(view, getParentFragmentManager());
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject == null)
      return;
    mController.updateView(mapObject.getReviewEditorAppName(), mapObject.getFeatureId());
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

}
