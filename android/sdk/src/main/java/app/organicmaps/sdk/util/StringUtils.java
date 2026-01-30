package app.organicmaps.sdk.util;

import android.content.Context;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Pair;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.R;
import java.text.NumberFormat;
import java.util.Locale;

public class StringUtils
{
  public static String toSnakeCase(String input)
  {
    return input.replaceAll("([a-z])([A-Z]+)", "$1_$2").toLowerCase();
  }
  public static String formatUsingUsLocale(String pattern, Object... args)
  {
    return String.format(Locale.US, pattern, args);
  }

  public static String formatUsingSystemLocale(String pattern, Object... args)
  {
    return String.format(Locale.getDefault(), pattern, args);
  }

  /**
   * This method returns correct string representation of % (percent number) for different locales
   * For example, that's how the result of  would look like:
   * — formatPercent(0.2319) will return 23.19% (in US locale)
   * — formatPercent(0.2319) will return %23.19 (in Turkish locale)
   * — formatPercent(0.2319) will return 23,19% (in Latvian locale)
   * — formatPercent(1.23) will return 123%
   * — formatPercent(0.000145) will return 0.01%
   * — formatPercent(0.37) will return 37%
   *
   * @param fraction a double value, that represents a fraction of a whole
   * @return correct string representation of percent for different locales
   */
  public static String formatPercent(double fraction)
  {
    NumberFormat percentFormat = NumberFormat.getPercentInstance();
    percentFormat.setMaximumFractionDigits(2);
    return percentFormat.format(fraction);
  }

  public static native boolean nativeIsHtml(String text);

  public static native boolean nativeContainsNormalized(String str, String substr);
  public static native String[] nativeFilterContainsNormalized(String[] strings, String substr);

  public static native int nativeFormatSpeed(double metersPerSecond);
  public static native Pair<String, String> nativeFormatSpeedAndUnits(double metersPerSecond);
  public static native Distance nativeFormatDistance(double meters);
  @NonNull
  public static native Pair<String, String> nativeGetLocalizedDistanceUnits();
  @NonNull
  public static native Pair<String, String> nativeGetLocalizedAltitudeUnits();
  @NonNull
  public static native String nativeGetLocalizedSpeedUnits();

  /**
   * Formats size in bytes to "x MB" or "x.x GB" format.
   * Small values rounded to 1 MB without fractions.
   * Decimal separator character depends on system locale.
   *
   * @param context context for getString()
   * @param size size in bytes
   * @return formatted string
   */
  public static String getFileSizeString(@NonNull Context context, long size)
  {
    if (size < Constants.GB)
    {
      int value = (int) ((float) size / Constants.MB + 0.5f);
      if (value == 0)
        value = 1;

      return formatUsingUsLocale("%1$d %2$s", value, context.getString(R.string.mb));
    }

    float value = ((float) size / Constants.GB);
    return formatUsingSystemLocale("%1$.1f %2$s", value, context.getString(R.string.gb));
  }

  public static boolean isRtl()
  {
    Locale defLocale = Locale.getDefault();
    return Character.getDirectionality(defLocale.getDisplayName(defLocale).charAt(0))
 == Character.DIRECTIONALITY_RIGHT_TO_LEFT;
  }

  /**
   * Formats an MWM version number (YYMMDD) into a locale-friendly short date string.
   * Examples: "12/31/25" (US), "31/12/25" (UK/EU), "25/12/31" (Japan)
   *
   * @param version version in YYMMDD format (e.g., 251231 for Dec 31, 2025)
   * @return formatted date string in locale's short format, or empty string if version is invalid
   */
  @NonNull
  public static String formatMwmVersion(long version)
  {
    if (version <= 0)
      return "";
    try
    {
      String v = String.valueOf(version);
      // Pad with leading zeros if needed (e.g., 10101 -> 010101)
      while (v.length() < 6)
        v = "0" + v;

      int year = Integer.parseInt(v.substring(0, 2));
      int month = Integer.parseInt(v.substring(2, 4));
      int day = Integer.parseInt(v.substring(4, 6));

      // Convert 2-digit year to full year (00-99 -> 2000-2099)
      java.util.Calendar cal = java.util.Calendar.getInstance();
      cal.set(2000 + year, month - 1, day);

      java.text.DateFormat df = java.text.DateFormat.getDateInstance(java.text.DateFormat.SHORT);
      return df.format(cal.getTime());
    }
    catch (Exception e)
    {
      return "";
    }
  }

  @NonNull
  public static String toLowerCase(@NonNull String string)
  {
    return string.toLowerCase(Locale.getDefault());
  }

  @NonNull
  public static String toUpperCase(@NonNull String string)
  {
    return string.toUpperCase(Locale.getDefault());
  }

  public static class SimpleTextWatcher implements TextWatcher
  {
    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after)
    {}

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {}

    @Override
    public void afterTextChanged(Editable s)
    {}
  }

  private StringUtils() {}
}
