package app.organicmaps.editor;

import static app.organicmaps.sdk.editor.data.Language.DEFAULT_LANG_CODE;
import static app.organicmaps.sdk.editor.data.Language.AUTO_LANG_CODE;

import android.content.res.Configuration;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.core.os.ConfigurationCompat;
import androidx.core.os.LocaleListCompat;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.data.Language;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.Set;

public class MapLanguagesFragment extends BaseMwmRecyclerFragment<LanguagesAdapter>
    implements LanguagesAdapter.OnLanguageSelectedListener
{
  final static String EXISTING_LOCALIZED_NAMES = "ExistingLocalizedNames";
  final static String IS_MAP_LANGUAGE_SELECTION = "IsMapLanguageSelection";

  public interface Listener
  {
    void onMapLanguageSelected(Language language);
  }

  private Listener mListener;

  @NonNull
  @Override
  protected LanguagesAdapter createAdapter()
  {
    Bundle args = getArguments();
    boolean isMapLanguageSelection = args != null ? args.getBoolean(IS_MAP_LANGUAGE_SELECTION) : true;
    Set<String> existingLanguages =
        args != null ? new HashSet<>(args.getStringArrayList(EXISTING_LOCALIZED_NAMES)) : new HashSet<>();

    Configuration config = requireContext().getResources().getConfiguration();
    LocaleListCompat systemLocales = ConfigurationCompat.getLocales(config);

    List<Language> languages = new ArrayList<>();
    List<Language> systemLanguages = new ArrayList<>(systemLocales.size());
    for (int i = 0; i < systemLocales.size(); i++)
      systemLanguages.add(null);

    // service languages are old_name, alt_name, ...
    boolean includeServiceLangs = !isMapLanguageSelection;

    for (Language lang : Editor.nativeGetSupportedLanguages(includeServiceLangs))
    {
      if (existingLanguages.contains(lang.code))
        continue;

      // Separately extract system languages
      for (int i = 0; i < systemLocales.size(); i++)
      {
        Locale locale = systemLocales.get(i);
        if (locale != null && locale.getLanguage().equals(lang.code))
        {
          systemLanguages.add(i, lang);
          break;
        }
      }

      if (systemLanguages.contains(lang))
        continue;

      languages.add(lang);
    }

    Collections.sort(languages, Comparator.comparing(lhs -> lhs.name));

    languages.addAll(0, systemLanguages.stream().filter(Objects::nonNull).toList());

    if (isMapLanguageSelection)
    {
      String localLanguageLabel = getString(R.string.pref_maplanguage_local);
      Language localLanguage = new Language(DEFAULT_LANG_CODE, localLanguageLabel);
      languages.add(0, localLanguage);

      String autoLanguageLabel = getString(R.string.auto);
      Language autoLanguage = new Language(AUTO_LANG_CODE, autoLanguageLabel);
      languages.add(0, autoLanguage);
    }

    return new LanguagesAdapter(this, languages.toArray(new Language[languages.size()]), null);
  }

  public void setListener(Listener listener)
  {
    this.mListener = listener;
  }

  @Override
  public void onLanguageSelected(Language language)
  {
    Fragment parent = getParentFragment();
    if (parent instanceof Listener)
      ((Listener) parent).onMapLanguageSelected(language);
    if (mListener != null)
      mListener.onMapLanguageSelected(language);
  }
}
