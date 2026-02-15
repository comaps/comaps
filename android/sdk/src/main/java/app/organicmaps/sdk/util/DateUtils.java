package app.organicmaps.sdk.util;

import android.content.Context;
import android.content.res.Resources;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.text.DateFormat;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.Locale;

import app.organicmaps.sdk.R;

public final class DateUtils
{
  private DateUtils() {}

  @NonNull
  public static DateFormat getShortDateFormatter()
  {
    return DateFormat.getDateInstance(DateFormat.SHORT, Locale.getDefault());
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public static boolean is24HourFormat(@NonNull Context context)
  {
    return android.text.format.DateFormat.is24HourFormat(context);
  }

  /***
   * @param dateString Date string in the yyyy-MM-dd format
   * @return Human-readable string of the time that's passed since the date
   */
  public static String getRelativePeriodString(Resources resources, String dateString)
  {
    DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd");
    int days = (int) (LocalDate.now().toEpochDay() - LocalDate.parse(dateString, formatter).toEpochDay());

    if (days == 0)
      return resources.getString(R.string.today).toLowerCase();
    if (days == 1)
      return resources.getString(R.string.yesterday).toLowerCase();
    if (days < 7)
      return  resources.getString(R.string.days_ago, Integer.toString(days));
    if (days < 30)
      return resources.getString(days < 14 ? R.string.week_ago : R.string.weeks_ago, Integer.toString(days / 7));
    if (days < 365)
      return resources.getString(days < 60 ? R.string.month_ago : R.string.months_ago, Integer.toString(days / 30));
    if (days > 365)
      return resources.getString(days < 730 ? R.string.year_ago : R.string.years_ago, Integer.toString(days / 365));
    else
      return "";
  }
}
