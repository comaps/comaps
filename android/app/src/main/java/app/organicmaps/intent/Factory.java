package app.organicmaps.intent;

import static app.organicmaps.api.Const.EXTRA_PICK_POINT;

import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.core.content.IntentCompat;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.editor.OsmLoginActivity;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.api.ParsedRoutingData;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.api.RoutePoint;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.FeatureId;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.util.StorageUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.downloader.CustomMwmManager;
import app.organicmaps.search.SearchActivity;
import java.io.File;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

public class Factory
{
  public static boolean isStartedForApiResult(@NonNull Intent intent)
  {
    // Previously, we relied on the implicit FORWARD_RESULT_FLAG to detect if the caller was
    // waiting for a result. However, this approach proved to be less reliable than using
    // the explicit EXTRA_PICK_POINT flag.
    // https://github.com/organicmaps/organicmaps/pull/8910
    return intent.getBooleanExtra(EXTRA_PICK_POINT, false);
  }

  public static class KmzKmlProcessor implements IntentProcessor
  {
    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      // See KML/KMZ/KMB intent filters in manifest.
      final List<Uri> uris;
      if (Intent.ACTION_VIEW.equals(intent.getAction()))
        uris = Collections.singletonList(intent.getData());
      else if (Intent.ACTION_SEND.equals(intent.getAction()))
        uris = Collections.singletonList(IntentCompat.getParcelableExtra(intent, Intent.EXTRA_STREAM, Uri.class));
      else if (Intent.ACTION_SEND_MULTIPLE.equals(intent.getAction()))
        uris = intent.getParcelableArrayListExtra(Intent.EXTRA_STREAM);
      else
        uris = null;
      if (uris == null)
        return false;

      MwmApplication app = MwmApplication.from(activity);
      final File tempDir = new File(StorageUtils.getTempPath(app));
      final ContentResolver resolver = activity.getContentResolver();
      ThreadPool.getStorage().execute(() -> BookmarkManager.INSTANCE.importBookmarksFiles(resolver, uris, tempDir));
      return false;
    }
  }

  public static class MwmFileProcessor implements IntentProcessor
  {
    private static final String MWM_EXTENSION = ".mwm";

    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity activity)
    {
      if (!Intent.ACTION_VIEW.equals(intent.getAction()))
        return false;

      final Uri uri = intent.getData();
      if (uri == null)
        return false;

      // Check if this is an MWM file
      String fileName = StorageUtils.getFileNameFromUri(activity.getContentResolver(), uri);
      if (fileName == null || !fileName.toLowerCase(Locale.US).endsWith(MWM_EXTENSION))
        return false;

      // Show spinner while copying (MWM files can be large)
      final ProgressDialog progressDialog = new ProgressDialog(activity, app.organicmaps.R.style.MwmTheme_ProgressDialog);
      progressDialog.setMessage(activity.getString(app.organicmaps.R.string.custom_mwm_importing));
      progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
      progressDialog.setIndeterminate(true);
      progressDialog.setCancelable(false);
      progressDialog.show();

      ThreadPool.getStorage().execute(() -> {
        CustomMwmManager.ImportResult result = CustomMwmManager.importMwmFile(activity, uri);

        activity.runOnUiThread(() -> {
          progressDialog.dismiss();
          switch (result)
          {
            case SUCCESS:
              android.widget.Toast.makeText(activity,
                  activity.getString(app.organicmaps.R.string.custom_mwm_import_success),
                  android.widget.Toast.LENGTH_SHORT).show();
              // Re-register maps in the C++ layer first, then recreate so the GL context
              // initialises with the newly registered custom map already present.
              Framework.nativeReloadWorldMaps();
              activity.recreate();
              break;
            case ERROR_INVALID_FILE:
              android.widget.Toast.makeText(activity,
                  activity.getString(app.organicmaps.R.string.custom_mwm_import_invalid),
                  android.widget.Toast.LENGTH_LONG).show();
              break;
            case ERROR_IO:
            case ERROR_STORAGE:
              android.widget.Toast.makeText(activity,
                  activity.getString(app.organicmaps.R.string.custom_mwm_import_error),
                  android.widget.Toast.LENGTH_LONG).show();
              break;
          }
        });
      });

      return true;
    }
  }

  public static class UrlProcessor implements IntentProcessor
  {
    private static final int SEARCH_IN_VIEWPORT_ZOOM = 16;

    @Override
    public boolean process(@NonNull Intent intent, @NonNull MwmActivity target)
    {
      final Uri uri = intent.getData();
      if (uri == null)
        return false;

      switch (Framework.nativeParseAndSetApiUrl(uri.toString()))
      {
      case RequestType.INCORRECT: return false;

      case RequestType.MAP:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        Map.executeMapApiRequest();
        return true;

      case RequestType.ROUTE:
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
        RoutingController.get().setRouterType(data.mRouterType);
        final RoutePoint from = data.mPoints[0];
        final RoutePoint to = data.mPoints[1];
        RoutingController.get().prepare(
            MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT, from.mName, "", from.mLat, from.mLon),
            MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT, to.mName, "", to.mLat, to.mLon));
        return true;
      case RequestType.SEARCH:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
        final double[] latlon = Framework.nativeGetParsedCenterLatLon();
        if (latlon != null)
        {
          Framework.nativeStopLocationFollow();
          Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
          // We need to update viewport for search api manually because of drape engine
          // will not notify subscribers when search activity is shown.
          if (!request.mIsSearchOnMap)
            Framework.nativeSetSearchViewport(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
        }
        SearchActivity.start(target, request.mQuery, request.mLocale, request.mIsSearchOnMap);
        return true;
      }
      case RequestType.CROSSHAIR:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();
        target.showPositionChooserForAPI(Framework.nativeGetParsedAppName());

        final double[] latlon = Framework.nativeGetParsedCenterLatLon();
        if (latlon != null)
        {
          Framework.nativeStopLocationFollow();
          Framework.nativeSetViewportCenter(latlon[0], latlon[1], SEARCH_IN_VIEWPORT_ZOOM);
        }

        return true;
      }
      case RequestType.OAUTH2:
      {
        SearchEngine.INSTANCE.cancelInteractiveSearch();

        final String oauth2code = Framework.nativeGetParsedOAuth2Code();
        OsmLoginActivity.OAuth2Callback(target, oauth2code);

        return true;
      }

      // Menu and Settings url types should be implemented to support deeplinking.
      case RequestType.MENU:
      case RequestType.SETTINGS:
      }

      return false;
    }
  }
}
