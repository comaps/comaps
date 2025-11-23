package app.organicmaps.editor;

import android.content.res.Configuration;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.core.os.ConfigurationCompat;
import androidx.core.os.LocaleListCompat;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.data.Language;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

public class LanguagesFragment extends BaseMwmRecyclerFragment<LanguagesAdapter>
{
  final static String EXISTING_LOCALIZED_NAMES = "ExistingLocalizedNames";

  public interface Listener
  {
    void onLanguageSelected(Language language);
  }

  private Listener mListener;

  @NonNull
  @Override
  protected LanguagesAdapter createAdapter()
  {
    Bundle args = getArguments();
    Set<String> existingLanguages =
        args != null ? new HashSet<>(args.getStringArrayList(EXISTING_LOCALIZED_NAMES)) : new HashSet<>();

    Configuration config = requireContext().getResources().getConfiguration();
    LocaleListCompat systemLocales = ConfigurationCompat.getLocales(config);

    List<Language> languages = new ArrayList<>();
    List<Language> systemLanguages = new ArrayList<>();

    for (Language lang : Editor.nativeGetSupportedLanguages(false))
    {
      // Separately extract system languages
      for (int i = 0; i < systemLocales.size(); i++)
      {
        Locale locale = systemLocales.get(i);
        if (locale != null && locale.getLanguage().equals(lang.code))
          systemLanguages.add(lang);
      }

      if (existingLanguages.contains(lang.code) || systemLanguages.contains(lang))
        continue;

      languages.add(lang);
    }

    Collections.sort(languages, Comparator.comparing(lhs -> lhs.name));

    languages.addAll(0, systemLanguages);

    return new LanguagesAdapter(this, languages.toArray(new Language[languages.size()]));
  }

  public void setListener(Listener listener)
  {
    this.mListener = listener;
  }

  protected void onLanguageSelected(Language language)
  {
    Fragment parent = getParentFragment();
    if (parent instanceof Listener)
      ((Listener) parent).onLanguageSelected(language);
    if (mListener != null)
      mListener.onLanguageSelected(language);
  }
}
