package app.organicmaps.settings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.ImageViewCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.settings.CyclingOptionsFragment;
import app.organicmaps.settings.DrivingOptionsFragment;
import app.organicmaps.settings.WalkingOptionsFragment;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class RoutingOptionsFragment extends BaseMwmToolbarFragment
{
  public static final String BUNDLE_ROAD_TYPES = "road_types";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_routing_options, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    Router routerType = RoutingController.get().getLastRouterType();
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                   ? makeRouteTypes(savedInstanceState)
                   : RoutingOptions.getActiveRoadTypes(routerType);

    ViewPager2 viewPager = view.findViewById(R.id.route_options_view_pager);
    OptionsPagerAdapter pagerAdapter = new OptionsPagerAdapter(this);
    viewPager.setAdapter(pagerAdapter);
    TabLayout tabLayout = view.findViewById(R.id.route_options_tab_layout);

    new TabLayoutMediator(tabLayout, viewPager, new TabLayoutMediator.TabConfigurationStrategy() {
      @Override
      public void onConfigureTab(@NonNull TabLayout.Tab tab, int position)
      {
        Context context = view.getContext();
        ImageView imageView = new ImageView(context);

        int sizeInPixels = (int) (48 * context.getResources().getDisplayMetrics().density);
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(sizeInPixels, sizeInPixels);
        imageView.setLayoutParams(params);

        switch (position)
        {
        case 0:
          imageView.setImageResource(R.drawable.ic_pedestrian);
          imageView.setContentDescription(getString(R.string.pedestrian));
          break;
        case 1:
          imageView.setImageResource(R.drawable.ic_bike);
          imageView.setContentDescription(getString(R.string.bicycle));
          break;
        case 2:
          imageView.setImageResource(R.drawable.ic_car);
          imageView.setContentDescription(getString(R.string.vehicle));
          break;
        }

        ColorStateList tabColors = tabLayout.getTabTextColors();
        if (tabColors != null)
        {
          ImageViewCompat.setImageTintList(imageView, tabColors);
        }
        tab.setCustomView(imageView);
      }
    }).attach();

    int index = 0;
    switch (RoutingController.get().getLastRouterType())
    {
    case Pedestrian: index = 0; break;
    case Bicycle: index = 1; break;
    case Vehicle: index = 2; break;
    case Transit: index = 0; break;
    }
    viewPager.setCurrentItem(index, false);
  }

  private class OptionsPagerAdapter extends FragmentStateAdapter
  {
    public OptionsPagerAdapter(Fragment f)
    {
      super(f);
    }

    private static final String TAG = OptionsPagerAdapter.class.getSimpleName();

    @Override
    public Fragment createFragment(int position)
    {
      switch (position)
      {
      case 0: return new WalkingOptionsFragment();
      case 1: return new CyclingOptionsFragment();
      case 2: return new DrivingOptionsFragment();
      default: Logger.w(TAG, "Invalid tab position: " + position); return new WalkingOptionsFragment();
      }
    }

    @Override
    public int getItemCount()
    {
      return 3; // walking, cycling, driving
    }
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
    {
      result.add(RoadType.values()[each]);
    }
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
    {
      savedRoadTypes.add(each.ordinal());
    }
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
  }

  private boolean areSettingsNotChanged()
  {
    Router routerType = RoutingController.get().getLastRouterType();
    Set<RoadType> lastActiveRoadTypes = RoutingOptions.getActiveRoadTypes(routerType);
    return mRoadTypes.equals(lastActiveRoadTypes);
  }

  @Override
  public boolean onBackPressed()
  {
    if (areSettingsNotChanged())
    {
      requireActivity().setResult(Activity.RESULT_CANCELED);
    }
    else
    {
      requireActivity().setResult(Activity.RESULT_OK);
      RoutingController.get().rebuildLastRoute();
    }

    return super.onBackPressed();
  }
}
