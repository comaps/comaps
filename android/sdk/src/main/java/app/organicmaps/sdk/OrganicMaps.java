package app.organicmaps.sdk;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import androidx.annotation.NonNull;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ProcessLifecycleOwner;
import androidx.preference.PreferenceManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.downloader.Android7RootCertificateWorkaround;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationProviderFactory;
import app.organicmaps.sdk.location.SensorHelper;
import app.organicmaps.sdk.maplayer.indoor.IndoorManager;
import app.organicmaps.sdk.maplayer.isolines.IsolinesManager;
import app.organicmaps.sdk.maplayer.subway.SubwayManager;
import app.organicmaps.sdk.maplayer.traffic.TrafficManager;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.settings.StoragePathManager;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.SharedPropertiesUtils;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.util.log.LogsManager;
import java.io.IOException;

public final class OrganicMaps implements DefaultLifecycleObserver
{
  private static final String TAG = OrganicMaps.class.getSimpleName();

  @NonNull
  private final String mFlavor;

  @NonNull
  private final Context mContext;

  @NonNull
  private final SharedPreferences mPreferences;

  @NonNull
  private final IsolinesManager mIsolinesManager;
  @NonNull
  private final IndoorManager mIndoorManager;
  @NonNull
  private final SubwayManager mSubwayManager;

  @NonNull
  private final LocationHelper mLocationHelper;
  @NonNull
  private final SensorHelper mSensorHelper;

  private volatile boolean mFrameworkInitialized;
  private volatile boolean mPlatformInitialized;

  @NonNull
  public LocationHelper getLocationHelper()
  {
    return mLocationHelper;
  }

  @NonNull
  public SensorHelper getSensorHelper()
  {
    return mSensorHelper;
  }

  @NonNull
  public SubwayManager getSubwayManager()
  {
    return mSubwayManager;
  }

  @NonNull
  public IsolinesManager getIsolinesManager()
  {
    return mIsolinesManager;
  }

  @NonNull
  public IndoorManager getIndoorManager()
  {
    return mIndoorManager;
  }

  public OrganicMaps(@NonNull Context context, @NonNull String flavor, @NonNull String applicationId, int versionCode,
                     @NonNull String versionName, @NonNull String fileProviderAuthority,
                     @NonNull LocationProviderFactory locationProviderFactory)
  {
    mFlavor = flavor;
    mContext = context.getApplicationContext();
    mPreferences = mContext.getSharedPreferences(context.getString(R.string.pref_file_name), Context.MODE_PRIVATE);

    // Set configuration directory as early as possible.
    // Other methods may explicitly use Config, which requires settingsDir to be set.
    final String settingsPath = StorageUtils.getSettingsPath(mContext);
    if (!StorageUtils.createDirectory(settingsPath))
      throw new AssertionError("Can't create settingsDir " + settingsPath);
    Logger.d(TAG, "Settings path = " + settingsPath);
    nativeSetSettingsDir(settingsPath);

    Config.init(mContext, mPreferences, flavor, applicationId, versionCode, versionName, fileProviderAuthority);
    OsmOAuth.init(mContext, mPreferences);
    SharedPropertiesUtils.init(mPreferences);
    LogsManager.INSTANCE.initFileLogging(mContext, mPreferences);

    Android7RootCertificateWorkaround.initializeIfNeeded(mContext);

    Icon.loadDefaultIcons(mContext.getResources(), mContext.getPackageName());

    mSensorHelper = new SensorHelper(mContext);
    mLocationHelper = new LocationHelper(mContext, mSensorHelper, locationProviderFactory);
    mIsolinesManager = new IsolinesManager();
    mIndoorManager = new IndoorManager();
    mSubwayManager = new SubwayManager(mContext);
  }

  /**
   * Initialize native core of application: platform and framework.
   *
   * @throws IOException - if failed to create directories. Caller must handle
   *                     the exception and do nothing with native code if initialization is failed.
   */
  public boolean init(@NonNull Runnable onComplete) throws IOException
  {
    initNativePlatform();
    return initNativeFramework(onComplete);
  }

  public boolean arePlatformAndCoreInitialized()
  {
    return mFrameworkInitialized && mPlatformInitialized;
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    nativeOnTransit(true);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    nativeOnTransit(false);
  }

  @NonNull
  public SharedPreferences getPreferences()
  {
    return mPreferences;
  }

  private void initNativePlatform() throws IOException
  {
    if (mPlatformInitialized)
      return;

    final String apkPath = StorageUtils.getApkPath(mContext);
    Logger.d(TAG, "Apk path = " + apkPath);
    // Note: StoragePathManager uses Config, which requires SettingsDir to be set.
    final String writablePath = StoragePathManager.findMapsStorage(mContext);
    Logger.d(TAG, "Writable path = " + writablePath);
    final String privatePath = StorageUtils.getPrivatePath(mContext);
    Logger.d(TAG, "Private path = " + privatePath);
    final String tempPath = StorageUtils.getTempPath(mContext);
    Logger.d(TAG, "Temp path = " + tempPath);

    // If platform directories are not created it means that native part of app will not be able
    // to work at all. So, we just ignore native part initialization in this case, e.g. when the
    // external storage is damaged or not available (read-only).
    createPlatformDirectories(writablePath, privatePath, tempPath);
    extractBundledMaps(writablePath, mContext.getAssets());

    nativeInitPlatform(mContext, apkPath, writablePath, privatePath, tempPath, mFlavor, BuildConfig.BUILD_TYPE,
                       /* isTablet */ false);
    Config.setStoragePath(writablePath);

    // Use the same prefs as SettingsPrefsFragment
    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
    final String savedUrl = prefs.getString(mContext.getString(R.string.pref_custom_map_download_url), "");
    Framework.nativeSetCustomMapDownloadUrl(savedUrl.trim());

    mPlatformInitialized = true;
    Logger.i(TAG, "Platform initialized");
  }

  private boolean initNativeFramework(@NonNull Runnable onComplete)
  {
    if (mFrameworkInitialized)
      return false;

    nativeInitFramework(onComplete);

    initNativeStrings();
    SearchEngine.INSTANCE.initialize();
    BookmarkManager.loadBookmarks();
    TtsPlayer.INSTANCE.initialize(mContext);
    RoutingController.get().initialize(mLocationHelper);

    TrafficManager.INSTANCE.initialize();
    mSubwayManager.initialize();
    mIsolinesManager.initialize();
    mIndoorManager.initialize();
    ProcessLifecycleOwner.get().getLifecycle().addObserver(this);

    Logger.i(TAG, "Framework initialized");
    mFrameworkInitialized = true;
    return true;
  }

  private void createPlatformDirectories(@NonNull String writablePath, @NonNull String privatePath,
                                         @NonNull String tempPath) throws IOException
  {
    SharedPropertiesUtils.emulateBadExternalStorage(mContext);

    StorageUtils.requireDirectory(writablePath);
    StorageUtils.requireDirectory(privatePath);
    StorageUtils.requireDirectory(tempPath);
  }

  private void initNativeStrings()
  {
    nativeAddLocalization("core_entrance", mContext.getString(R.string.core_entrance));
    nativeAddLocalization("core_exit", mContext.getString(R.string.core_exit));
    nativeAddLocalization("core_my_places", mContext.getString(R.string.core_my_places));
    nativeAddLocalization("core_my_position", mContext.getString(R.string.core_my_position));
    nativeAddLocalization("core_placepage_unknown_place", mContext.getString(R.string.core_placepage_unknown_place));
    nativeAddLocalization("postal_code", mContext.getString(R.string.postal_code));
    nativeAddLocalization("wifi", mContext.getString(R.string.category_wifi));
  }

  /** @TODO: REMOVE BUNDLED MWM LOGIC AND INCLUDES BEFORE MERGE
   * Copies any .mwm files bundled in APK assets (except World/WorldCoasts, which C++ handles) into
   * the versioned maps directory so FindAllLocalMapsAndCleanup can discover them on first launch.
   */
  private void extractBundledMaps(@NonNull String writablePath, @NonNull AssetManager assets)
  {
    try {
      long version = readCountriesVersion(assets);
      if (version <= 0) {
        Logger.w(TAG, "extractBundledMaps: could not read countries version");
        return;
      }
      // Extract to the current version dir using real countries.txt IDs so Storage registers
      // them in m_localFiles (not m_localFilesForFakeCountries) and the download prompt is
      // suppressed. For real country IDs, the size-mismatch check only logs LWARNING, no crash.
      String dirVersion = String.valueOf(version);
      File versionDir = new File(writablePath, dirVersion);
      if (!versionDir.exists() && !versionDir.mkdirs()) {
        Logger.e(TAG, "extractBundledMaps: failed to create " + versionDir);
        return;
      }

      // Maps asset filename → countries.txt country ID (= destination filename).
      Map<String, String> bundledMaps = new LinkedHashMap<>();
      bundledMaps.put("Berlin.mwm",        "Germany_Berlin.mwm");
      bundledMaps.put("SanFrancisco.mwm",   "US_California_Santa_Clara_Palo Alto.mwm");
      bundledMaps.put("SantaRosa.mwm",      "US_California_Chico.mwm");
      bundledMaps.put("France_Ile-de-France_Seine-et-Marne.mwm", "France_Ile-de-France_Seine-et-Marne.mwm");
      bundledMaps.put("Salem.mwm",          "US_Oregon_Portland.mwm");

      // Remove stale copies (old fake names or old version dirs) from every version directory
      // so Storage never encounters a fake country at m_currentVersion (which would crash).
      File writableDir = new File(writablePath);
      File[] siblings = writableDir.listFiles(File::isDirectory);
      if (siblings != null) {
        for (File sibling : siblings) {
          for (Map.Entry<String, String> e : bundledMaps.entrySet()) {
            for (String name : new String[]{e.getKey(), e.getValue()}) {
              File stale = new File(sibling, name);
              if (!stale.equals(new File(versionDir, e.getValue())) && stale.exists()) {
                Logger.i(TAG, "extractBundledMaps: removing stale " + stale);
                stale.delete();
              }
            }
          }
        }
      }

      for (Map.Entry<String, String> e : bundledMaps.entrySet()) {
        File dest = new File(versionDir, e.getValue());
        if (dest.exists()) continue;
        Logger.i(TAG, "extractBundledMaps: extracting " + e.getKey() + " as " + e.getValue());
        try (InputStream in = assets.open(e.getKey());
             FileOutputStream out = new FileOutputStream(dest)) {
          byte[] buf = new byte[65536];
          int n;
          while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
        Logger.i(TAG, "extractBundledMaps: done " + e.getValue() + " (" + dest.length() + " bytes)");
      }
    } catch (IOException e) {
      Logger.e(TAG, "extractBundledMaps: " + e.getMessage());
    }
  }

  private long readCountriesVersion(@NonNull AssetManager assets)
  {
    try (InputStream in = assets.open("countries.txt");
         BufferedReader reader = new BufferedReader(new InputStreamReader(in))) {
      char[] buf = new char[1024];
      int n = reader.read(buf);
      Matcher m = Pattern.compile("\"v\"\\s*:\\s*(\\d+)").matcher(new String(buf, 0, n));
      if (m.find()) return Long.parseLong(m.group(1));
    } catch (IOException e) {
      Logger.e(TAG, "readCountriesVersion: " + e.getMessage());
    }
    return -1;
  }

  /**
   * This method stores Context that is used by C++. Active Activity must be update this on every
   * onResume(), or otherwise strings in C++ won't be localised into correct language. Also, don't
   * forget to use nativeSetContext() in onPause() to pass ApplicationContext, otherwise there will
   * be memory leak or crash.
   *
   * @param context must be Activity or Fragment Context ()
   */
  public static native void nativeSetContext(Context context);

  private static native void nativeSetSettingsDir(String settingsPath);

  private static native void nativeInitPlatform(Context context, String apkPath, String writablePath,
                                                String privatePath, String tmpPath, String flavorName, String buildType,
                                                boolean isTablet);

  private static native void nativeInitFramework(@NonNull Runnable onComplete);

  private static native void nativeAddLocalization(String name, String value);

  private static native void nativeOnTransit(boolean foreground);

  static
  {
    System.loadLibrary("organicmaps");
  }
}
