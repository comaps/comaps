package app.organicmaps.search;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.util.Graphics;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;
import java.util.ArrayList;
import java.util.List;

class TabAdapter extends FragmentStateAdapter
{
  enum Tab
  {
    HISTORY {
      @Override
      public int getTitleRes()
      {
        return R.string.history;
      }

      @Override
      public Class<? extends Fragment> getFragmentClass()
      {
        return SearchHistoryFragment.class;
      }
    },

    CATEGORIES {
      @Override
      public int getTitleRes()
      {
        return R.string.categories;
      }

      @Override
      public Class<? extends Fragment> getFragmentClass()
      {
        return SearchCategoriesFragment.class;
      }
    };

    public abstract int getTitleRes();
    public abstract Class<? extends Fragment> getFragmentClass();
  }

  interface OnTabSelectedListener
  {
    void onTabSelected(@NonNull Tab tab);
  }

  private final ViewPager2 mPager;
  private final List<Tab> mTabs_list = new ArrayList<>();
  private final Context mContext;
  private OnTabSelectedListener mTabSelectedListener;
  private final TabLayout mTabs;
  private final Fragment mFragment;
  TabAdapter(@NonNull Fragment fragment, ViewPager2 pager, TabLayout tabs)
  {
    super(fragment);
    this.mFragment = fragment;
    this.mPager = pager;
    this.mContext = pager.getContext();
    this.mTabs = tabs;
    for (Tab tab : Tab.values())
    {
      if (tab == Tab.HISTORY && !Config.isSearchHistoryEnabled())
        continue;
      mTabs_list.add(tab);
    }

    mPager.setAdapter(this);
    if (mTabs.getVisibility() != View.GONE)
      attachTo(tabs);
  }

  private void attachTo(TabLayout tabs)
  {
    new TabLayoutMediator(tabs, mPager, (tab, position) ->
      tab.setText(mTabs_list.get(position).getTitleRes())).attach();

    tabs.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
      @Override
      public void onTabSelected(TabLayout.Tab tab)
      {
        SharedPreferences.Editor editor = MwmApplication.prefs(mContext).edit();
        editor.putInt(Config.KEY_PREF_LAST_SEARCHED_TAB, tab.getPosition());
        editor.apply();
        if (tab.getIcon() != null)
          Graphics.tint(mContext, tab.getIcon(), com.google.android.material.R.attr.colorSecondary);
        if (mTabSelectedListener != null)
          mTabSelectedListener.onTabSelected(mTabs_list.get(tab.getPosition()));
      }
      @Override
      public void onTabUnselected(TabLayout.Tab tab)
      {
        if (tab.getIcon() != null)
          Graphics.tint(mContext, tab.getIcon());
      }
      @Override
      public void onTabReselected(TabLayout.Tab tab)
      {}
    });

    SharedPreferences preferences = MwmApplication.prefs(mPager.getContext());
    int lastSelectedTabPosition = preferences.getInt(Config.KEY_PREF_LAST_SEARCHED_TAB, 0);
    mPager.setCurrentItem(lastSelectedTabPosition, false);
  }

  void setTabSelectedListener(OnTabSelectedListener listener)
  {
    mTabSelectedListener = listener;
  }

  @NonNull
  @Override
  public Fragment createFragment(int position)
  {
    return mFragment.getChildFragmentManager().getFragmentFactory().instantiate(mContext.getClassLoader(),
                                                   mTabs_list.get(position).getFragmentClass().getName());
  }

  @Override
  public int getItemCount()
  {
    return mTabs_list.size();
  }
}