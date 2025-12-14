package app.organicmaps.sdk.util;

import android.content.Context;
import android.content.res.Resources;
import android.os.Build;
import android.os.LocaleList;
import android.text.TextUtils;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class Language
{
  // Locale.getLanguage() returns even 3-letter codes, not that we need in the C++ core,
  // so we use locale itself, like zh_CN
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String getDefaultLocale()
  {
    String lang = Locale.getDefault().toString();
    if (TextUtils.isEmpty(lang))
      return Locale.US.toString();

    // Replace deprecated language codes:
    // in* -> id
    // iw* -> he
    if (lang.startsWith("in"))
      return "id";
    if (lang.startsWith("iw"))
      return "he";

    return lang;
  }

  @Keep
  @SuppressWarnings("unused")
  @NonNull
  public static String[] getSystemLocales()
  {
    List<String> result = new ArrayList<>();

    // Check for Android version high enough to support Locale Preference list
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
    {
      // Gets user language preference list, on system level - app-specific changes are not applied
      // (For that a Context object would be necessary)
      LocaleList list = Resources.getSystem().getConfiguration().getLocales();
      for (int i = 0; i < list.size(); i++)
      {
        String lang = list.get(i).toString();
        if (lang.startsWith("in"))
          lang = "id";
        if (lang.startsWith("iw"))
          lang = "he";
        result.add(lang);
      }
    }
    else
    {
      result.add(Locale.getDefault().toString());
    }
    return result.toArray(new String[0]);
  }

  // After some testing on Galaxy S4, looks like this method doesn't work on all devices:
  // sometime it always returns the same value as getDefaultLocale()
  @NonNull
  public static String getKeyboardLocale(@NonNull Context context)
  {
    InputMethodManager imm = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
    if (imm == null)
      return getDefaultLocale();

    final InputMethodSubtype ims = imm.getCurrentInputMethodSubtype();
    if (ims == null)
      return getDefaultLocale();

    String locale = ims.getLocale();
    if (TextUtils.isEmpty(locale.trim()))
      return getDefaultLocale();

    return locale;
  }

  @NonNull
  public static native String nativeNormalize(@NonNull String locale);
}
