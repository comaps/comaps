package app.organicmaps.widget.placepage.sections;

import android.animation.ValueAnimator;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.editor.data.TimeFormatUtils;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.editor.OpeningHours;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.sdk.util.DateUtils;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.textview.MaterialTextView;
import java.util.Calendar;
import java.util.Locale;

public class PlacePageOpeningHoursFragment extends Fragment implements Observer<MapObject>
{
  private View mFrame;
  private MaterialTextView mTodayLabel;
  private RecyclerView mFullWeekOpeningHours;
  private MaterialTextView mLastCheckedDate;
  private PlaceOpeningHoursAdapter mOpeningHoursAdapter;
  private LinearLayout mTodayShiftsLayout;
  private View dropDownIcon;
  private boolean isOhExpanded;

  private PlacePageViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_opening_hours_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mFrame = view;
    mTodayLabel = view.findViewById(R.id.oh_today_label);
    mTodayShiftsLayout = view.findViewById(R.id.oh_today_shifts);
    mFullWeekOpeningHours = view.findViewById(R.id.rw__full_opening_hours);
    mLastCheckedDate = view.findViewById(R.id.oh_check_date);
    mOpeningHoursAdapter = new PlaceOpeningHoursAdapter();
    mFullWeekOpeningHours.setAdapter(mOpeningHoursAdapter);
    dropDownIcon = view.findViewById(R.id.dropdown_icon);
    mFullWeekOpeningHours.getLayoutParams().height = 0;
    UiUtils.hide(dropDownIcon);
    isOhExpanded = false;
  }

  private static void setOrHideLastCheckedDate(MapObject mapObject, Resources resources, MaterialTextView checkDateView)
  {
    final String checkDate = mapObject.getMetadata(Metadata.MetadataType.FMD_CHECK_DATE_OPEN_HOURS);
    if (!checkDate.isEmpty())
    {
      String periodSinceCheck = DateUtils.getRelativePeriodString(resources, checkDate);
      UiUtils.setTextAndShow(checkDateView, resources.getString(R.string.hours_confirmed_time_ago, periodSinceCheck));
    }
    else
      UiUtils.hide(checkDateView);
  }

  private void refreshTodayOpeningHours(String label, String[] shifts, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayLabel, label);
    mTodayLabel.setTextColor(color);
    mTodayShiftsLayout.removeAllViews();
    mTodayShiftsLayout.setVisibility(View.VISIBLE);
    for (String shift : shifts)
    {
      MaterialTextView tv = new MaterialTextView(requireContext());
      tv.setText(shift);
      tv.setTextSize(TypedValue.COMPLEX_UNIT_PX, mTodayLabel.getTextSize());
      tv.setTypeface(mTodayLabel.getTypeface());
      tv.setTextColor(color);
      tv.setTextAlignment(View.TEXT_ALIGNMENT_VIEW_END);
      mTodayShiftsLayout.addView(tv);
    }
  }

  private void refreshTodayOpeningHours(String label, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayLabel, label);
    mTodayLabel.setTextColor(color);
    mTodayShiftsLayout.removeAllViews();
    mTodayShiftsLayout.setVisibility(View.GONE);
  }

  private void refreshOpeningHours(MapObject mapObject)
  {
    final String ohStr = mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
    mFrame.setOnLongClickListener((v) -> {
      PlacePageUtils.copyToClipboard(requireContext(), mFrame,
                                     TimeFormatUtils.formatTimetables(getResources(), ohStr, timetables).toString());
      return true;
    });

    final boolean isEmptyTT = (timetables == null || timetables.length == 0);
    final int color = ThemeUtils.getColor(requireContext(), android.R.attr.textColorPrimary);
    final Resources resources = getResources();

    setOrHideLastCheckedDate(mapObject, resources, mLastCheckedDate);

    if (isEmptyTT)
    {
      // 'opening_hours' tag wasn't parsed either because it's empty or wrong format.
      if (!ohStr.isEmpty())
      {
        UiUtils.show(mFrame);
        refreshTodayOpeningHours(ohStr, color);
        UiUtils.hide(mFullWeekOpeningHours);
      }
      else
        UiUtils.hide(mFrame);
    }
    else
    {
      UiUtils.show(mFrame);
      if (timetables[0].isFullWeek())
      {
        final Timetable tt = timetables[0];
        if (tt.isFullday)
        {
          refreshTodayOpeningHours(resources.getString(R.string.twentyfour_seven), color);
        }
        else
        {
          refreshTodayOpeningHours(resources.getString(R.string.daily), TimeFormatUtils.getShiftStrings(tt), color);
        }
        UiUtils.hide(mFullWeekOpeningHours);
      }
      else
      {
        // Show whole week timetable.
        int firstDayOfWeek = Calendar.getInstance(Locale.getDefault()).getFirstDayOfWeek();
        mOpeningHoursAdapter.setTimetables(timetables, firstDayOfWeek);
        final View iconView = mFrame.findViewById(R.id.dropdown_icon);
        UiUtils.show(iconView);
        mFrame.setOnClickListener((v) -> expandOpeningHours());
        mFullWeekOpeningHours.addOnItemTouchListener(new RecyclerView.OnItemTouchListener() {
          @Override
          public boolean onInterceptTouchEvent(@NonNull RecyclerView rv, @NonNull MotionEvent e)
          {
            if (e.getAction() == MotionEvent.ACTION_UP)
              expandOpeningHours();
            return false;
          }
          @Override
          public void onTouchEvent(@NonNull RecyclerView rv, @NonNull MotionEvent e)
          {}
          @Override
          public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept)
          {}
        });
        // Show today's open time + non-business time.
        boolean containsCurrentWeekday = false;
        final int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
        for (Timetable tt : timetables)
        {
          if (tt.containsWeekday(currentDay))
          {
            containsCurrentWeekday = true;
            String[] shifts;

            if (tt.isFullday)
            {
              shifts = new String[] {resources.getString(R.string.editor_time_allday)};
            }
            else
            {
              shifts = TimeFormatUtils.getShiftStrings(tt);
            }
            refreshTodayOpeningHours(resources.getString(app.organicmaps.sdk.R.string.today), shifts, color);
            break;
          }
        }

        // Show that place is closed today.
        if (!containsCurrentWeekday)
        {
          refreshTodayOpeningHours(resources.getString(R.string.day_off_today),
                                   ContextCompat.getColor(requireContext(), R.color.base_red));
        }
      }
    }
  }

  private void expandOpeningHours()
  {
    int targetHeight, startHeight;
    if (!isOhExpanded)
    {
      UiUtils.show(mFullWeekOpeningHours);
      startHeight = 0;
      mFullWeekOpeningHours.measure(View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
                                    View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
      targetHeight = mFullWeekOpeningHours.getMeasuredHeight();
      dropDownIcon.animate().rotation(-180f).setDuration(200).start();
      isOhExpanded = true;
    }
    else
    {
      startHeight = mFullWeekOpeningHours.getMeasuredHeight();
      targetHeight = 0;
      dropDownIcon.animate().rotation(0f).setDuration(200).start();
      isOhExpanded = false;
    }
    mFullWeekOpeningHours.getLayoutParams().height = startHeight;
    final ValueAnimator va = ValueAnimator.ofInt(startHeight, targetHeight);
    va.setDuration(200);
    va.addUpdateListener(animation -> {
      mFullWeekOpeningHours.getLayoutParams().height = (int) animation.getAnimatedValue();
      mFullWeekOpeningHours.requestLayout();
    });
    va.start();
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
      refreshOpeningHours(mapObject);
  }
}
