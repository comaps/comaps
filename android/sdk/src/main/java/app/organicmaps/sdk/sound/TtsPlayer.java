package app.organicmaps.sdk.sound;

import android.content.Context;
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.speech.tts.Voice;
import android.text.TextUtils;
import android.util.Pair;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.os.ConfigurationCompat;
import androidx.core.os.LocaleListCompat;
import androidx.media.AudioAttributesCompat;
import androidx.media.AudioFocusRequestCompat;
import androidx.media.AudioManagerCompat;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * {@code TtsPlayer} class manages available TTS voice languages.
 * Single TTS language is described by {@link LanguageData} item.
 * <p>
 * We support a set of languages listed in {@code strings-tts.xml} file.
 * During loading each item in this list is marked as {@code downloaded} or {@code not downloaded},
 * unsupported voices are excluded.
 * <p>
 * At startup we check whether currently selected language is in our list of supported voices and its data is
 * downloaded. If not, we check system default locale. If failed, the same check is made for other system locales.
 * If those fail too, we check for English language. Then, as a final resort, all installed TTS locales are checked.
 * <p> If no core supported languages can be used by the system, TTS is locked down and can not be
 * enabled and used.
 */
public enum TtsPlayer
{
  INSTANCE;

  private static final String TAG = TtsPlayer.class.getSimpleName();
  private static final Locale DEFAULT_LOCALE = Locale.US;
  private static final float SPEECH_RATE = 1.0f;
  private static final int TTS_SPEAK_DELAY_MILLIS = 50;

  @Nullable
  private static List<Pair<String, String>> sSupportedLanguages = null;

  public static Runnable sOnReloadCallback = null;

  private ContentObserver mTtsEngineObserver;
  private TextToSpeech mTts;
  private boolean mInitializing;
  private boolean mReloadTriggered = false;

  private final AudioFocusRequestCompat mAudioFocusRequest =
      new AudioFocusRequestCompat.Builder(AudioManagerCompat.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK)
          .setAudioAttributes(new AudioAttributesCompat.Builder()
                                  .setUsage(AudioAttributesCompat.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                                  .setContentType(AudioAttributesCompat.CONTENT_TYPE_SPEECH)
                                  .build())
          .setOnAudioFocusChangeListener(focusChange -> {})
          .build();
  private AudioManager mAudioManager;

  private final Bundle mParams = new Bundle();

  private final Handler delayHandler = new Handler(Looper.getMainLooper());

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  // TTS is locked down due to absence of supported languages
  private boolean mUnavailable;

  private LocaleListCompat mInstalledSystemLocales;

  TtsPlayer() {}

  private static @Nullable LanguageData findSupportedLanguage(String internalCode, List<LanguageData> langs)
  {
    if (TextUtils.isEmpty(internalCode))
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesInternalCode(internalCode))
        return lang;

    return null;
  }

  private static @Nullable LanguageData findSupportedLanguage(Locale locale, List<LanguageData> langs)
  {
    if (locale == null)
      return null;

    for (LanguageData lang : langs)
      if (lang.matchesLocale(locale))
        return lang;

    return null;
  }

  private boolean setLanguageInternal(LanguageData lang)
  {
    try
    {
      mTts.setLanguage(lang.locale);
      nativeSetTurnNotificationsLocale(lang.internalCode);
      Config.TTS.setLanguage(lang.internalCode);

      return true;
    }
    catch (IllegalArgumentException e)
    {
      lockDown();
      return false;
    }
  }

  public boolean setLanguage(LanguageData lang)
  {
    return (lang != null && setLanguageInternal(lang));
  }

  public static @Nullable LanguageData getSelectedLanguage(List<LanguageData> langs)
  {
    return findSupportedLanguage(Config.TTS.getLanguage(), langs);
  }

  private @Nullable LanguageData getSystemLanguage(List<LanguageData> langs)
  {
    LanguageData res;

    // Try default system locale
    Locale defLocale = Locale.getDefault();
    res = findSupportedLanguage(defLocale, langs);
    if (res != null && res.downloaded)
      return res;

    // Try other installed system locales
    for (int i = 0; i < mInstalledSystemLocales.size(); i++)
    {
      Locale loc = mInstalledSystemLocales.get(i);
      res = findSupportedLanguage(loc, langs);
      if (res != null && res.downloaded)
        return res;
    }
    return null;
  }

  private @Nullable LanguageData getTTSLanguage(List<LanguageData> langs)
  {
    LanguageData res;

    // Try all TTS installed languages
    Set<Locale> ttsLocales = mTts.getAvailableLanguages();
    for (Locale loc : ttsLocales)
    {
      res = findSupportedLanguage(loc, langs);
      if (res != null && res.downloaded)
        return res;
    }

    return null;
  }

  private static @Nullable LanguageData getDefaultLanguage(List<LanguageData> langs)
  {
    LanguageData res;

    // Try default app locale (en.US)
    res = findSupportedLanguage(DEFAULT_LOCALE, langs);
    if (res != null && res.downloaded)
      return res;
    return null;
  }

  private void lockDown()
  {
    mUnavailable = true;
    setEnabled(false);
  }

  public void initialize(@NonNull Context context)
  {
    mContext = context;

    if (mTts != null || mInitializing || mUnavailable)
      return;

    mInitializing = true;
    // TextToSpeech.OnInitListener() can be called from a non-main thread
    // on LineageOS '20.0-20231127-RELEASE-thyme' 'Xiaomi/thyme/thyme'.
    // https://github.com/organicmaps/organicmaps/issues/6903

    Configuration config = context.getResources().getConfiguration();
    mInstalledSystemLocales = ConfigurationCompat.getLocales(config);

    mTts = new TextToSpeech(context, status -> UiThread.run(() -> {
      if (status == TextToSpeech.ERROR)
      {
        Logger.e(TAG, "Failed to initialize TextToSpeech");
        lockDown();
        mInitializing = false;
        return;
      }
      refreshLanguages();
      mTts.setSpeechRate(SPEECH_RATE);
      mTts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
        @Override
        public void onStart(String utteranceId)
        {
          AudioManagerCompat.requestAudioFocus(mAudioManager, mAudioFocusRequest);
        }

        @Override
        public void onDone(String utteranceId)
        {
          AudioManagerCompat.abandonAudioFocusRequest(mAudioManager, mAudioFocusRequest);
        }

        @Override
        @SuppressWarnings("deprecated") // abstract method must be implemented
        public void onError(String utteranceId)
        {
          AudioManagerCompat.abandonAudioFocusRequest(mAudioManager, mAudioFocusRequest);
        }

        @Override
        public void onError(String utteranceId, int errorCode)
        {
          AudioManagerCompat.abandonAudioFocusRequest(mAudioManager, mAudioFocusRequest);
        }
      });
      mAudioManager = ContextCompat.getSystemService(context, AudioManager.class);
      mParams.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, Config.TTS.getVolume());
      mInitializing = false;
      if (mReloadTriggered && sOnReloadCallback != null)
      {
        sOnReloadCallback.run();
        mReloadTriggered = false;
      }
    }));

    if (mTtsEngineObserver == null)
    {
      mTtsEngineObserver = new ContentObserver(new Handler(Looper.getMainLooper())) {
        @Override
        public void onChange(boolean selfChange)
        {
          Logger.d(TAG, "System TTS engine changed – reloading TTS engine");
          mReloadTriggered = true;
          if (mTts != null)
          {
            mTts.shutdown();
            mTts = null;
          }
          initialize(mContext);
        }
      };
      mContext.getContentResolver().registerContentObserver(Settings.Secure.getUriFor("tts_default_synth"), false,
                                                            mTtsEngineObserver);
    }
  }

  public static boolean isReady()
  {
    return (INSTANCE.mTts != null && !INSTANCE.mUnavailable && !INSTANCE.mInitializing);
  }

  public Locale getVoiceLocale()
  {
    if (INSTANCE.mTts == null)
      return null;
    Voice voice = INSTANCE.mTts.getVoice();
    return voice != null ? voice.getLocale() : null;
  }

  public String getLanguageDisplayName()
  {
    Locale locale = getVoiceLocale();
    return locale != null ? locale.getDisplayName(locale) : null;
  }

  public void speak(String textToSpeak)
  {
    if (Config.TTS.isEnabled())
      try
      {
        final boolean isMusicActive = mAudioManager.isMusicActive();
        AudioManagerCompat.requestAudioFocus(mAudioManager, mAudioFocusRequest);
        final long delay = isMusicActive ? TTS_SPEAK_DELAY_MILLIS : 0;
        delayHandler.postDelayed(() -> mTts.speak(textToSpeak, TextToSpeech.QUEUE_ADD, mParams, textToSpeak), delay);
      }
      catch (IllegalArgumentException e)
      {
        lockDown();
      }
  }

  public void playTurnNotifications(@NonNull String[] turnNotifications)
  {
    if (isReady())
      for (String textToSpeak : turnNotifications)
        speak(textToSpeak);
  }

  public void stop()
  {
    if (isReady())
      try
      {
        AudioManagerCompat.abandonAudioFocusRequest(mAudioManager, mAudioFocusRequest);
        mTts.stop();
      }
      catch (IllegalArgumentException e)
      {
        lockDown();
      }
  }

  public static boolean isEnabled()
  {
    return (isReady() && nativeAreTurnNotificationsEnabled());
  }

  public static void setEnabled(boolean enabled)
  {
    Config.TTS.setEnabled(enabled);
    nativeEnableTurnNotifications(enabled);
  }

  public float getVolume()
  {
    return Config.TTS.getVolume();
  }

  public void setVolume(final float volume)
  {
    mParams.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, volume);
    Config.TTS.setVolume(volume);
  }

  private boolean getUsableLanguages(List<LanguageData> outList)
  {
    for (final Pair<String, String> langNamePair : getSupportedLanguages())
    {
      try
      {
        outList.add(new LanguageData(langNamePair.first, langNamePair.second, mTts));
      }
      catch (LanguageData.NotAvailableException ex)
      {
        Logger.w(TAG, "Failed to get usable languages " + ex.getMessage());
      }
      catch (IllegalArgumentException e)
      {
        Logger.e(TAG, "Failed to get usable languages", e);
        lockDown();
        return false;
      }
    }

    return true;
  }

  private @Nullable LanguageData refreshLanguagesInternal(List<LanguageData> outList)
  {
    if (!getUsableLanguages(outList))
      return null;

    if (outList.isEmpty())
    {
      Logger.d("TtsPlayer", "No supported languages found, lock down TTS :( ");
      lockDown();
      return null;
    }

    LanguageData res = getSelectedLanguage(outList);
    if (res != null && res.downloaded)
    {
      Logger.d("TtsPlayer", "Selected locale " + res.internalCode + " will be used for TTS");
      return res;
    }
    Logger.d("TtsPlayer", "Selected locale " + Config.TTS.getLanguage()
                              + " is not available or not downloaded, trying system locales...");

    res = getSystemLanguage(outList);
    if (res != null && res.downloaded)
    {
      Logger.d("TtsPlayer", "System locale " + res.internalCode + " will be used for TTS");
      return res;
    }
    Logger.d("TtsPlayer",
             "None of the system locales are available, or they are not downloaded, trying default locale...");

    res = getDefaultLanguage(outList);
    if (res != null && res.downloaded)
    {
      Logger.d("TtsPlayer", "Default locale " + res.internalCode + " will be used for TTS");
      return res;
    }
    Logger.d("TtsPlayer",
             "Default locale " + DEFAULT_LOCALE + " can not be used either, trying all installed TTS locales...");

    res = getTTSLanguage(outList);
    if (res != null && res.downloaded)
    {
      Logger.d("TtsPlayer", "TTS locale " + res.internalCode + " will be used for TTS");
      return res;
    }

    Logger.d("TtsPlayer",
             "None of the TTS engine locales are available, or they are not downloaded, disabling TTS :( ");
    Config.TTS.setEnabled(false);
    return null;
  }

  public @NonNull List<LanguageData> refreshLanguages()
  {
    List<LanguageData> res = new ArrayList<>();
    if (mUnavailable || mTts == null)
      return res;

    LanguageData lang = refreshLanguagesInternal(res);
    setLanguage(lang);

    setEnabled(Config.TTS.isEnabled());
    return res;
  }

  @NonNull
  private List<Pair<String, String>> getSupportedLanguages()
  {
    if (sSupportedLanguages == null)
    {
      sSupportedLanguages = nativeGetSupportedLanguages();
    }
    return sSupportedLanguages;
  }

  private native static void nativeEnableTurnNotifications(boolean enable);
  private native static boolean nativeAreTurnNotificationsEnabled();
  private native static void nativeSetTurnNotificationsLocale(String code);
  private native static String nativeGetTurnNotificationsLocale();
  @NonNull
  private native static List<Pair<String, String>> nativeGetSupportedLanguages();
}
