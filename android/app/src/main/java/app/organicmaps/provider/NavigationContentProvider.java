package app.organicmaps.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.log.Logger;

public class NavigationContentProvider extends ContentProvider
{

  private static final String TAG = NavigationContentProvider.class.getSimpleName();
  private static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);
  public static final int LIVE_NAVIGATION_DATA_CODE = 1;
  static {
    URI_MATCHER.addURI(NavigationContract.AUTHORITY, "live", LIVE_NAVIGATION_DATA_CODE);
  }

  @Override
  public boolean onCreate() {
    return true;
  }

  @Override
  public String getType(@NonNull Uri uri) {
    return switch (URI_MATCHER.match(uri)) {
      case LIVE_NAVIGATION_DATA_CODE -> "vnd.android.cursor.item/app.comaps.navigation.routinginfo";
      default -> null;
    };
  }

  @Nullable
  @Override
  public Cursor query(@NonNull Uri uri, @Nullable String[] projection,
                      @Nullable String selection, @Nullable String[] selectionArgs,
                      @Nullable String sortOrder)
  {
    if (URI_MATCHER.match(uri) != LIVE_NAVIGATION_DATA_CODE)
    {
      Logger.w(TAG, "Unknown URI: " + uri);
      return null;
    }

    String[] columns = projection != null ? projection : new String[0];
    RoutingInfo info = RoutingController.get().getCachedRoutingInfo();
    if (info == null)
    {
      return new MatrixCursor(columns, 0);
    }

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
