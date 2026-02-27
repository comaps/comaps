package app.organicmaps.sdk.editor.data;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

// Corresponds to localisation::Language in core.
// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class Language
{
  // localisation::GetLanguageNameByLanguageIndex(localisation::kDefaultNameIndex).
  public static final String DEFAULT_LANG_CODE = "default";
  public static final String AUTO_LANG_CODE = "auto";

  public final String code;
  public final String name;

  public Language(@NonNull String code, @NonNull String name)
  {
    this.code = code;
    this.name = name;
  }
}
