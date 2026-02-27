package app.organicmaps.widget.placepage.sections;

import static app.organicmaps.editor.data.TimeFormatUtils.formatWeekdaysRange;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.Timetable;
import com.google.android.material.textview.MaterialTextView;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.List;
public class PlaceOpeningHoursAdapter extends RecyclerView.Adapter<PlaceOpeningHoursAdapter.ViewHolder>
{
  private List<WeekScheduleData> mWeekSchedule = Collections.emptyList();

  public PlaceOpeningHoursAdapter() {}

  public PlaceOpeningHoursAdapter(Timetable[] timetables, int firstDayOfWeek)
  {
    setTimetables(timetables, firstDayOfWeek);
  }

  public void setTimetables(Timetable[] timetables, int firstDayOfWeek)
  {
    final List<Integer> weekDays = buildWeekByFirstDay(firstDayOfWeek);
    final List<WeekScheduleData> scheduleData = new ArrayList<>();

    // Timetables array contains only working days. We need to fill non-working gaps.
    for (int i = 0; i < weekDays.size(); i++)
    {
      final int weekDay = weekDays.get(i);

      final Timetable tt = findScheduleForWeekDay(timetables, weekDay);
      final int startWeekDay = weekDays.get(i);
      if (tt != null)
      {
        while (i < weekDays.size() && tt.containsWeekday(weekDays.get(i)))
          i++;

        i--;
        final int endWeekDay = weekDays.get(i);
        scheduleData.add(new WeekScheduleData(startWeekDay, endWeekDay, tt));
      }
      else
      {
        // Search next working day in timetables.
        while (i + 1 < weekDays.size())
        {
          if (findScheduleForWeekDay(timetables, weekDays.get(i + 1)) != null)
            break;
          i++;
        }

        scheduleData.add(new WeekScheduleData(startWeekDay, weekDays.get(i), null));
      }
    }

    mWeekSchedule = scheduleData;

    notifyDataSetChanged();
  }

  public static List<Integer> buildWeekByFirstDay(int firstDayOfWeek)
  {
    if (firstDayOfWeek < 1 || firstDayOfWeek > 7)
      throw new IllegalArgumentException("First day of week " + firstDayOfWeek + " is out of range [1..7]");

    final List<Integer> list = Arrays.asList(Calendar.SUNDAY, Calendar.MONDAY, Calendar.TUESDAY, Calendar.WEDNESDAY,
                                             Calendar.THURSDAY, Calendar.FRIDAY, Calendar.SATURDAY);
    Collections.rotate(list, 1 - firstDayOfWeek);
    return list;
  }

  public static Timetable findScheduleForWeekDay(Timetable[] tables, int weekDay)
  {
    for (Timetable tt : tables)
      if (tt.containsWeekday(weekDay))
        return tt;

    return null;
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return new ViewHolder(
        LayoutInflater.from(parent.getContext()).inflate(R.layout.place_page_opening_hours_item, parent, false));
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position)
  {
    if (mWeekSchedule == null || position >= mWeekSchedule.size() || position < 0)
      return;

    final WeekScheduleData schedule = mWeekSchedule.get(position);

    if (schedule.isClosed)
    {
      holder.setWeekdays(formatWeekdaysRange(schedule.startWeekDay, schedule.endWeekDay));
      holder.setOpenTime(holder.itemView.getResources().getString(R.string.day_off));
      return;
    }

    final Timetable tt = schedule.timetable;
    holder.setWeekdays(formatWeekdaysRange(schedule.startWeekDay, schedule.endWeekDay));
    if (tt.isFullday)
    {
      holder.setOpenTime(holder.itemView.getResources().getString(R.string.editor_time_allday));
    }
    else
    {
      if (tt.closedTimespans == null)
        return;

      if (tt.closedTimespans.length == 0)
      {
        holder.setOpenTime(tt.workingTimespan.toWideString());
      }
      else
      {
        StringBuilder openings = new StringBuilder();
        openings.append(tt.workingTimespan.start).append(" – ").append(tt.closedTimespans[0].start);

        for (int i = 0; i < tt.closedTimespans.length - 1; i++)
        {
          openings.append("\n").append(tt.closedTimespans[i].end).append(" – ").append(tt.closedTimespans[i + 1].start);
        }

        openings.append("\n")
            .append(tt.closedTimespans[tt.closedTimespans.length - 1].end)
            .append(" – ")
            .append(tt.workingTimespan.end);

        holder.setOpenTime(openings.toString());
      }
    }
  }

  @Override
  public int getItemCount()
  {
    return (mWeekSchedule != null ? mWeekSchedule.size() : 0);
  }

  public static class WeekScheduleData
  {
    public final int startWeekDay;
    public final int endWeekDay;
    public final boolean isClosed;
    public final Timetable timetable;

    public WeekScheduleData(int startWeekDay, int endWeekDay, Timetable timetable)
    {
      this.startWeekDay = startWeekDay;
      this.endWeekDay = endWeekDay;
      this.isClosed = timetable == null;
      this.timetable = timetable;
    }
  }

  public static class ViewHolder extends RecyclerView.ViewHolder
  {
    private final MaterialTextView mWeekdays;
    private final MaterialTextView mOpenTime;

    public ViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mWeekdays = itemView.findViewById(R.id.tv__opening_hours_weekdays);
      mOpenTime = itemView.findViewById(R.id.tv__opening_hours_time);
      itemView.setVisibility(View.VISIBLE);
    }

    public void setWeekdays(String weekdays)
    {
      mWeekdays.setText(weekdays);
    }

    public void setOpenTime(String openTime)
    {
      mOpenTime.setText(openTime);
    }
  }
}
