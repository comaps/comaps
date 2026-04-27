package app.organicmaps.settings;

import android.content.res.XmlResourceParser;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.os.LocaleListCompat;

import org.xmlpull.v1.XmlPullParser;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.editor.LanguagesAdapter;
import app.organicmaps.sdk.editor.data.Language;

/**
 * Fragment for selecting app UI language from available translations.
 * Uses locales defined in res/xml/locales_config.xml.
 */
public class AppLanguagesFragment extends BaseMwmRecyclerFragment<LanguagesAdapter>
    implements LanguagesAdapter.OnLanguageSelectedListener
{
  public interface Listener
  {
    void onAppLanguageSelected();
  }

  private Listener mListener;
  private Map<Language, Locale> mLocaleLanguageMap;

  @NonNull
  @Override
  protected LanguagesAdapter createAdapter()
  {
    List<Locale> supportedLocales = parseSupportedLocales();

    mLocaleLanguageMap = new HashMap<>();
    List<Language> languages = new ArrayList<>();
    for (Locale locale : supportedLocales)
    {
      String localeTag = locale.toLanguageTag();
      String displayName = locale.getDisplayName(locale);
      Language language = new Language(localeTag, displayName);
      mLocaleLanguageMap.put(language, locale);
      languages.add(language);
    }

    languages.sort(Comparator.comparing(lhs -> lhs.name));
    languages.add(0, createSystemDefault());

    Language currentLanguage = getCurrentAppLanguage();
    return new LanguagesAdapter(this, languages.toArray(new Language[0]), currentLanguage);
  }

  private Language createSystemDefault()
  {
    return new Language(Language.DEFAULT_LANG_CODE, getString(R.string.setting_value_system_default));
  }

  @NonNull
  private Language getCurrentAppLanguage()
  {
    LocaleListCompat appLocales = AppCompatDelegate.getApplicationLocales();
    if (appLocales.isEmpty())
      return createSystemDefault();
    Locale currentLocale = appLocales.get(0);
    if (currentLocale == null)
      throw new IllegalStateException("Cannot be null");
    String localeTag = currentLocale.toLanguageTag();
    String displayName = currentLocale.getDisplayName(currentLocale);
    return new Language(localeTag, displayName);
  }

  /**
   * Parse supported locales from res/xml/locales_config.xml.
   */
  private List<Locale> parseSupportedLocales()
  {
    List<Locale> locales = new ArrayList<>();
    try (XmlResourceParser parser = getResources().getXml(R.xml.locales_config))
    {
      int eventType = parser.getEventType();
      while (eventType != XmlPullParser.END_DOCUMENT)
      {
        if (eventType == XmlPullParser.START_TAG && "locale".equals(parser.getName()))
        {
          String localeName = parser.getAttributeValue(
              "http://schemas.android.com/apk/res/android", "name");
          if (localeName != null && !localeName.isEmpty())
            locales.add(Locale.forLanguageTag(localeName));
        }
        eventType = parser.next();
      }
    }
    catch (Exception e)
    {
      // Fallback: return empty list, which means no languages available
    }
    return locales;
  }

  public void setListener(Listener listener)
  {
    mListener = listener;
  }

  @Override
  public void onLanguageSelected(Language language)
  {
    LocaleListCompat localeList;
    if (language.code.equals(Language.DEFAULT_LANG_CODE))
      localeList = LocaleListCompat.getEmptyLocaleList();
    else
      localeList = LocaleListCompat.create(mLocaleLanguageMap.get(language));

    AppCompatDelegate.setApplicationLocales(localeList);
    if (mListener != null) mListener.onAppLanguageSelected();
  }
}
