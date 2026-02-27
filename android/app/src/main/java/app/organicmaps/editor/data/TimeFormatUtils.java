package app.organicmaps.editor.data;

import android.content.res.Resources;
import android.graphics.Typeface;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.util.Utils;
import java.text.DateFormatSymbols;
import java.util.Locale;

public class TimeFormatUtils
{
  private TimeFormatUtils() {}

  private static String[] sShortWeekdays;
  private static Locale sCurrentLocale;

  private static void refreshWithCurrentLocale()
  {
    if (!Locale.getDefault().equals(sCurrentLocale))
    {
      sCurrentLocale = Locale.getDefault();
      sShortWeekdays = DateFormatSymbols.getInstance().getShortWeekdays();
      for (int i = 0; i < sShortWeekdays.length; i++)
      {
        sShortWeekdays[i] = Utils.capitalize(sShortWeekdays[i]);
      }
    }
  }

  public static String formatShortWeekday(@IntRange(from = 1, to = 7) int day)
  {
    refreshWithCurrentLocale();
    return sShortWeekdays[day];
  }

  public static String formatWeekdaysRange(int startWeekDay, int endWeekDay)
  {
    refreshWithCurrentLocale();
    if (startWeekDay == endWeekDay)
      return sShortWeekdays[startWeekDay];
    else
      return sShortWeekdays[startWeekDay] + "-" + sShortWeekdays[endWeekDay];
  }

  public static String formatWeekdays(@NonNull Timetable timetable)
  {
    return formatWeekdays(timetable.weekdays);
  }

  public static String formatWeekdays(@NonNull int[] weekdays)
  {
    if (weekdays.length == 0)
      return "";

    refreshWithCurrentLocale();
    final StringBuilder builder = new StringBuilder(sShortWeekdays[weekdays[0]]);
    boolean iteratingRange;
    for (int i = 1; i < weekdays.length;)
    {
      iteratingRange = (weekdays[i] == weekdays[i - 1] + 1);
      if (iteratingRange)
      {
        while (i < weekdays.length && weekdays[i] == weekdays[i - 1] + 1)
          i++;
        builder.append("-").append(sShortWeekdays[weekdays[i - 1]]);
        continue;
      }

      if (i < weekdays.length)
        builder.append(", ").append(sShortWeekdays[weekdays[i]]);

      i++;
    }

    return builder.toString();
  }

  public static CharSequence formatTimetables(@NonNull Resources resources, String ohStr, Timetable[] timetables)
  {
    if (timetables == null || timetables.length == 0)
      return ohStr;

    // Generate string "24/7" or "Daily HH:MM - HH:MM".
    if (timetables[0].isFullWeek())
    {
      Timetable tt = timetables[0];

      String dailyStr = resources.getString(R.string.daily);
      SpannableStringBuilder ssb = new SpannableStringBuilder();

      if (tt.isFullday)
        return resources.getString(R.string.twentyfour_seven);
      else if (tt.closedTimespans == null || tt.closedTimespans.length == 0)
        ssb.append(dailyStr).append("\n").append(tt.workingTimespan.toWideString());
      else
        ssb.append(dailyStr).append("\n").append(getOpeningHours(tt));

      ssb.setSpan(new StyleSpan(Typeface.BOLD), 0, dailyStr.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      return ssb;
    }

    // Generate full week multiline string. E.g.
    // "Mon-Fri HH:MM - HH:MM
    // Sat HH:MM - HH:MM"
    SpannableStringBuilder weekSchedule = new SpannableStringBuilder();
    boolean firstRow = true;
    int currentOffset = 0;
    for (Timetable tt : timetables)
    {
      if (!firstRow)
      {
        weekSchedule.append('\n');
        currentOffset += 1;
      }

      final String weekdays = formatWeekdays(tt);
      String openTime;
      if (tt.isFullday)
      {
        openTime = resources.getString(R.string.editor_time_allday);
      }
      else if (tt.closedTimespans.length == 0)
      {
        openTime = tt.workingTimespan.toWideString();
      }
      else
      {
        openTime = getOpeningHours(tt);
      }

      int weekdaysStart = currentOffset;
      int weekdaysEnd = currentOffset + weekdays.length();

      weekSchedule.append(weekdays).append(' ').append('\n').append(openTime);

      currentOffset += weekdays.length() + 2 + openTime.length();
      weekSchedule.setSpan(new StyleSpan(Typeface.BOLD), weekdaysStart, weekdaysEnd, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

      firstRow = false;
    }

    return weekSchedule;
  }

  public static String getOpeningHours(Timetable tt)
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

    return openings.toString();
  }
}
