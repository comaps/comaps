package app.organicmaps.editor.data;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.util.Utils;
import java.text.DateFormatSymbols;
import java.util.Calendar;
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

  // OSM weekday codes -> java.util.Calendar day-of-week (1 = Sunday).
  private static int osmDayToCalendarDay(String day)
  {
    switch (day)
    {
      case "Mo": return 2;
      case "Tu": return 3;
      case "We": return 4;
      case "Th": return 5;
      case "Fr": return 6;
      case "Sa": return 7;
      case "Su": return 1;
      default: return -1;
    }
  }

  // Converts OSM weekday tokens to localized names; returns input if unparsable.
  private static String formatCollectionDays(String days)
  {
    refreshWithCurrentLocale();
    StringBuilder result = new StringBuilder();
    for (String range : days.split(","))
    {
      String r = range.trim();
      if (r.isEmpty())
        continue;
      if (result.length() > 0)
        result.append(", ");

      String[] ends = r.split("-");
      int start = osmDayToCalendarDay(ends[0]);
      int end = ends.length > 1 ? osmDayToCalendarDay(ends[1]) : start;
      if (start < 0 || end < 0)
        return days; // Not a weekday expression, keep as-is.
      result.append(formatWeekdaysRange(start, end));
    }
    return result.toString();
  }

  // Formats "14:00" per the device time preference (12h vs 24h). Returns input if unparsable.
  private static String formatCollectionTime(@NonNull Context context, @NonNull String time)
  {
    String[] hm = time.split(":");
    if (hm.length != 2)
      return time;
    try
    {
      int hours = Integer.parseInt(hm[0].trim());
      int minutes = Integer.parseInt(hm[1].trim());
      if (hours < 0 || hours > 24 || minutes < 0 || minutes > 59)
        return time;
      Calendar c = Calendar.getInstance();
      c.set(Calendar.HOUR_OF_DAY, hours);
      c.set(Calendar.MINUTE, minutes);
      c.set(Calendar.SECOND, 0);
      c.set(Calendar.MILLISECOND, 0);
      return android.text.format.DateFormat.getTimeFormat(context).format(c.getTime());
    }
    catch (NumberFormatException e)
    {
      return time;
    }
  }

  // Handles "14:00", "14:00-16:00" and "09:00, 17:00".
  private static String formatCollectionTimePart(@NonNull Context context, @NonNull String timePart)
  {
    if (timePart.contains("-"))
    {
      String[] ends = timePart.split("-");
      if (ends.length == 2)
        return formatCollectionTime(context, ends[0]) + "-" + formatCollectionTime(context, ends[1]);
    }
    StringBuilder result = new StringBuilder();
    for (String t : timePart.split(","))
    {
      String time = t.trim();
      if (time.isEmpty())
        continue;
      if (result.length() > 0)
        result.append(", ");
      result.append(formatCollectionTime(context, time));
    }
    return result.length() > 0 ? result.toString() : timePart;
  }

  // "Mo-Fr 14:00; Sa 09:00" -> "Mon-Fri 2:00 PM · Sat 9:00 AM", times per device preference.
  public static String formatCollectionTimes(@NonNull Context context, @NonNull String raw)
  {
    if (raw.isEmpty())
      return raw;

    refreshWithCurrentLocale();
    StringBuilder result = new StringBuilder();
    for (String part : raw.split(";"))
    {
      String group = part.trim();
      if (group.isEmpty())
        continue;

      int i = 0;
      while (i < group.length() && !Character.isDigit(group.charAt(i)))
        i++;
      String days = group.substring(0, i).trim();
      String time = group.substring(i).trim();
      String formattedGroup;
      if (!days.isEmpty() && !time.isEmpty())
        formattedGroup = formatCollectionDays(days) + " " + formatCollectionTimePart(context, time);
      else if (!days.isEmpty())
        formattedGroup = formatCollectionDays(days);
      else
        formattedGroup = formatCollectionTimePart(context, time);

      if (result.length() > 0)
        result.append(" · ");
      result.append(formattedGroup);
    }
    return result.length() > 0 ? result.toString() : raw;
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
        ssb.append(dailyStr).append("\n").append(TextUtils.join("\n", getShiftStrings(tt)));

      ssb.setSpan(new StyleSpan(Typeface.BOLD), 0, dailyStr.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      return ssb;
    }

    // Generate full week multiline string. E.g.
    // "Mon-Fri HH:MM - HH:MM
    // Sat HH:MM - HH:MM"
    SpannableStringBuilder weekSchedule = new SpannableStringBuilder();
    boolean firstRow = true;
    for (Timetable tt : timetables)
    {
      if (!firstRow)
      {
        weekSchedule.append('\n');
      }

      final String weekdays = formatWeekdays(tt);
      String openTime;
      if (tt.isFullday)
      {
        openTime = resources.getString(R.string.editor_time_allday);
      }
      else if (tt.closedTimespans == null || tt.closedTimespans.length == 0)
      {
        openTime = tt.workingTimespan.toWideString();
      }
      else
      {
        openTime = TextUtils.join("\n", getShiftStrings(tt));
      }

      int weekdaysStart = weekSchedule.length();
      weekSchedule.append(weekdays);
      int weekdaysEnd = weekSchedule.length();

      weekSchedule.append('\n').append(openTime);

      weekSchedule.setSpan(new StyleSpan(Typeface.BOLD), weekdaysStart, weekdaysEnd, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

      firstRow = false;
    }

    return weekSchedule;
  }
  public static String[] getShiftStrings(Timetable tt)
  {
    if (tt.closedTimespans == null || tt.closedTimespans.length == 0)
      return new String[] {tt.workingTimespan.toWideString()};
    String[] shifts = new String[tt.closedTimespans.length + 1];
    shifts[0] = tt.workingTimespan.start + "—" + tt.closedTimespans[0].start;
    for (int i = 0; i < tt.closedTimespans.length - 1; i++)
      shifts[i + 1] = tt.closedTimespans[i].end + "—" + tt.closedTimespans[i + 1].start;
    shifts[tt.closedTimespans.length] =
        tt.closedTimespans[tt.closedTimespans.length - 1].end + "—" + tt.workingTimespan.end;
    return shifts;
  }
}
