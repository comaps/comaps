package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Locale;

public class OpenStateTextFormatterTest
{
  private static final String OPENS_AT = "Opens at %s";
  private static final String CLOSES_AT = "Closes at %s";
  private static final String OPENS_DAY_AT = "Opens %1$s at %2$s";
  private static final String CLOSES_DAY_AT = "Closes %1$s at %2$s";

  @Test
  public void formatHoursMinutes_24h()
  {
    assertEquals("09:00", OpenStateTextFormatter.formatHoursMinutes(9, 0, true));
    assertEquals("18:05", OpenStateTextFormatter.formatHoursMinutes(18, 5, true));
  }

  @Test
  public void formatHoursMinutes_12h()
  {
    assertEquals("9:00 AM", OpenStateTextFormatter.formatHoursMinutes(9, 0, false));
    assertEquals("6:05 PM", OpenStateTextFormatter.formatHoursMinutes(18, 5, false));
    assertEquals("12:00 PM", OpenStateTextFormatter.formatHoursMinutes(12, 0, false));
    assertEquals("12:00 AM", OpenStateTextFormatter.formatHoursMinutes(0, 0, false));
  }

  @Test
  public void buildAtLabel_today_open_close()
  {
    String open = OpenStateTextFormatter.buildAtLabel(true, true, "Sat", "09:00",
        OPENS_AT, CLOSES_AT, OPENS_DAY_AT, CLOSES_DAY_AT);
    String close = OpenStateTextFormatter.buildAtLabel(false, true, "Sat", "18:00",
        OPENS_AT, CLOSES_AT, OPENS_DAY_AT, CLOSES_DAY_AT);
    assertEquals("Opens at 09:00", open);
    assertEquals("Closes at 18:00", close);
  }

  @Test
  public void buildAtLabel_other_day()
  {
    String open = OpenStateTextFormatter.buildAtLabel(true, false, "Sat", "09:00",
        OPENS_AT, CLOSES_AT, OPENS_DAY_AT, CLOSES_DAY_AT);
    String close = OpenStateTextFormatter.buildAtLabel(false, false, "Tue", "18:00",
        OPENS_AT, CLOSES_AT, OPENS_DAY_AT, CLOSES_DAY_AT);
    assertEquals("Opens Sat at 09:00", open);
    assertEquals("Closes Tue at 18:00", close);
  }

  @Test
  public void isSameLocalDate_and_dayShort_helpers()
  {
    ZonedDateTime a = ZonedDateTime.of(2025, 3, 1, 10, 0, 0, 0, ZoneId.of("Europe/Paris"));
    ZonedDateTime b = ZonedDateTime.of(2025, 3, 1, 22, 0, 0, 0, ZoneId.of("Europe/Paris"));
    ZonedDateTime c = a.plusDays(1);

    assertTrue(OpenStateTextFormatter.isSameLocalDate(a, b));
    assertFalse(OpenStateTextFormatter.isSameLocalDate(a, c));

    String day = OpenStateTextFormatter.dayShort(c, Locale.US);
    // March 2, 2025 is a Sunday; "Sun" in US locale
    assertEquals("Sun", day);
  }
}
