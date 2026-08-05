package app.organicmaps.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.routing.RouteGeometry;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.log.Logger;

public class NavigationContentProvider extends ContentProvider
{

  private static final String TAG = NavigationContentProvider.class.getSimpleName();
  private static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);
  public static final int LIVE_NAVIGATION_DATA_CODE = 1;
  public static final int ROUTE_GEOMETRY_CODE = 2;
  static {
    URI_MATCHER.addURI(NavigationContract.AUTHORITY, "live", LIVE_NAVIGATION_DATA_CODE);
    URI_MATCHER.addURI(NavigationContract.AUTHORITY, "route", ROUTE_GEOMETRY_CODE);
  }

  private final RoutingController.RouteChangedListener mRouteChangedListener = this::onRouteChanged;

  @Override
  public boolean onCreate() {
    RoutingController.get().addRouteChangedListener(mRouteChangedListener);
    return true;
  }

  @Override
  public void shutdown() {
    RoutingController.get().removeRouteChangedListener(mRouteChangedListener);
    super.shutdown();
  }

  private void onRouteChanged() {
    final Context context = getContext();
    if (context == null)
      return;
    context.getContentResolver().notifyChange(NavigationContract.LIVE_NAVIGATION_DATA_URI, null);
    context.getContentResolver().notifyChange(NavigationContract.ROUTE_GEOMETRY_URI, null);
  }

  @Override
  public String getType(@NonNull Uri uri) {
    return switch (URI_MATCHER.match(uri)) {
      case LIVE_NAVIGATION_DATA_CODE -> "vnd.android.cursor.item/app.comaps.navigation.routinginfo";
      case ROUTE_GEOMETRY_CODE -> "vnd.android.cursor.dir/app.comaps.navigation.routepoint";
      default -> null;
    };
  }

  @Nullable
  @Override
  public Cursor query(@NonNull Uri uri, @Nullable String[] projection,
                      @Nullable String selection, @Nullable String[] selectionArgs,
                      @Nullable String sortOrder)
  {
    final int match = URI_MATCHER.match(uri);
    if (match == ROUTE_GEOMETRY_CODE)
      return queryRouteGeometry(projection);

    if (match != LIVE_NAVIGATION_DATA_CODE)
    {
      Logger.w(TAG, "Unknown URI: " + uri);
      return null;
    }

    String[] columns = projection != null ? projection : NavigationContract.Live.ALL_COLUMNS;
    RoutingInfo info = RoutingController.get().getCachedRoutingInfo();
    if (info == null)
    {
      return new MatrixCursor(columns, 0);
    }

    final RouteGeometry geometry = RoutingController.get().getRouteGeometry();

    MatrixCursor cursor = new MatrixCursor(columns, 1);
    MatrixCursor.RowBuilder row = cursor.newRow();

    // Right now the DIST_TO_X columns will always return as formatted.
    // In the future, we can add query parameters to allow consuming apps to specify what unit
    // to receive DIST_TO_X in, similar to Breezy Weather:
    // https://github.com/breezy-weather/breezy-weather-data-sharing-lib#weather

    for (String column : columns)
    {
      switch (column)
      {
        case NavigationContract.Live.Columns.SESSION_STATE:
          row.add(column, info.routingSessionState.name());
          break;
        case NavigationContract.Live.Columns.CAR_DIRECTION:
          row.add(column, info.carDirection.name());
          break;
        case NavigationContract.Live.Columns.PEDESTRIAN_DIRECTION:
          row.add(column, info.pedestrianTurnDirection.name());
          break;
        case NavigationContract.Live.Columns.DIST_TO_TURN:
          row.add(column, info.distToTurn.toString(getContext()));
          break;
        case NavigationContract.Live.Columns.DIST_TO_TARGET:
          row.add(column, info.distToTarget.toString(getContext()));
          break;
        case NavigationContract.Live.Columns.DIST_TO_NEXT_STOP:
          row.add(column, info.distToNextStop.toString(getContext()));
          break;
        case NavigationContract.Live.Columns.TOTAL_TIME_SECONDS:
          row.add(column, info.totalTimeInSeconds);
          break;
        case NavigationContract.Live.Columns.TIME_TO_NEXT_STOP:
          row.add(column, info.timeToNextStop);
          break;
        case NavigationContract.Live.Columns.CURRENT_STREET:
          row.add(column, info.currentStreet);
          break;
        case NavigationContract.Live.Columns.NEXT_STREET:
          row.add(column, info.nextStreet);
          break;
        case NavigationContract.Live.Columns.COMPLETION_PERCENT:
          row.add(column, info.completionPercent);
          break;
        case NavigationContract.Live.Columns.EXIT_NUM:
          row.add(column, info.exitNum);
          break;
        case NavigationContract.Live.Columns.ROUTE_REVISION:
          row.add(column, geometry.mRevision);
          break;
        case NavigationContract.Live.Columns.ROUTE_POINT_COUNT:
          row.add(column, geometry.getPointCount());
          break;
        case NavigationContract.Live.Columns.DEST_LAT:
          row.add(column, geometry.hasDestination() ? geometry.mDestLat : null);
          break;
        case NavigationContract.Live.Columns.DEST_LON:
          row.add(column, geometry.hasDestination() ? geometry.mDestLon : null);
          break;
        case NavigationContract.Live.Columns.DEST_TITLE:
          row.add(column, geometry.mDestTitle);
          break;
      }
    }
    return cursor;
  }

  @NonNull
  private Cursor queryRouteGeometry(@Nullable String[] projection)
  {
    final String[] columns = projection != null ? projection : NavigationContract.Route.ALL_COLUMNS;
    final RouteGeometry geometry = RoutingController.get().getRouteGeometry();
    final int points = geometry.getPointCount();

    final MatrixCursor cursor = new MatrixCursor(columns, points);
    for (int i = 0; i < points; ++i)
    {
      final MatrixCursor.RowBuilder row = cursor.newRow();
      for (String column : columns)
      {
        switch (column)
        {
          case NavigationContract.Route.Columns.SEQ:
            row.add(column, i);
            break;
          case NavigationContract.Route.Columns.LAT:
            row.add(column, geometry.mJunctions[i].mLat);
            break;
          case NavigationContract.Route.Columns.LON:
            row.add(column, geometry.mJunctions[i].mLon);
            break;
          case NavigationContract.Route.Columns.REVISION:
            row.add(column, geometry.mRevision);
            break;
        }
      }
    }
    return cursor;
  }

  @Nullable
  @Override
  public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
    // no-op
    return null;
  }

  @Override
  public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
    // no-op
    return 0;
  }

  @Override
  public int delete(@NonNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
    // no-op
    return 0;
  }
}
