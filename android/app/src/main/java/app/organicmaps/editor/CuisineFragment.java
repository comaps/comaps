package app.organicmaps.editor;

import androidx.annotation.NonNull;

import java.util.Arrays;

import app.organicmaps.base.BaseMwmRecyclerFragment;

public class CuisineFragment extends BaseMwmRecyclerFragment<CuisineAdapter>
{
  private CuisineAdapter mAdapter;

  private String[] mInitialSelectedKeys;

  @NonNull
  @Override
  protected CuisineAdapter createAdapter()
  {
    mAdapter = new CuisineAdapter();
    mInitialSelectedKeys = getCuisines().clone();
    return mAdapter;
  }

  @NonNull
  public String[] getCuisines()
  {
    return mAdapter.getCuisines();
  }

  public void resetCuisines()
  {
    mAdapter.resetCuisines();
  }

  public boolean hasUnsavedChanges()
  {
    String[] initial = mInitialSelectedKeys;
    String[] current = getCuisines().clone();
    if (initial.length != current.length)
      return true;

    Arrays.sort(initial);
    Arrays.sort(current);
    return !Arrays.equals(initial, current);
  }

  public void setFilter(String filter)
  {
    mAdapter.setFilter(filter);
  }
}
