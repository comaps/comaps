package app.organicmaps.widget.placepage;

import java.time.ZonedDateTime;
import java.time.format.TextStyle;
import java.util.Locale;

public class OpenStateTextFormatter
{
  private OpenStateTextFormatter() {}

  static String formatHoursMinutes(int hour, int minute, boolean use24h)
  {
    if (use24h)
      return String.format(Locale.ROOT, "%02d:%02d", hour, minute);

    int h = hour % 12;
    if (h == 0) h = 12;
    String ampm = (hour < 12) ? "AM" : "PM";
    return String.format(Locale.ROOT, "%d:%02d %s", h, minute, ampm);
  }

  static boolean isSameLocalDate(ZonedDateTime a, ZonedDateTime b)
  {
    return a.toLocalDate().isEqual(b.toLocalDate());
  }

  static String dayShort(ZonedDateTime t, Locale locale)
  {
    return t.getDayOfWeek().getDisplayName(TextStyle.SHORT, locale);
  }

  static String buildAtLabel(
      boolean opens,
      boolean isToday,
      String dayShort,
      String time,
      String opensAtLocalized,
      String closesAtLocalized,
      String opensDayAtLocalized,
      String closesDayAtLocalized
  )
  {
    if (isToday)
      return opens ? String.format(Locale.ROOT, opensAtLocalized, time) // Opens at %s
          : String.format(Locale.ROOT, closesAtLocalized, time); // Closes at %s
    return opens ? String.format(Locale.ROOT, opensDayAtLocalized, dayShort, time) // Opens %s at %s
        : String.format(Locale.ROOT, closesDayAtLocalized, dayShort, time); // Closes %s at %s
  }
}
