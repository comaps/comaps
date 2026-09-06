package app.organicmaps.provider;

import android.net.Uri;

import app.organicmaps.BuildConfig;

/**
 * Constants for the NavigationContentProvider.
 */
public final class NavigationContract
{
  public static final String AUTHORITY = BuildConfig.APPLICATION_ID + ".provider.navigation";
  public static final Uri LIVE_NAVIGATION_DATA_URI = Uri.parse("content://" + AUTHORITY + "/live");

  public static final Uri ROUTE_GEOMETRY_URI = Uri.parse("content://" + AUTHORITY + "/route");

  public static final class Route {
    public static final String[] ALL_COLUMNS = {
            NavigationContract.Route.Columns.SEQ,
            NavigationContract.Route.Columns.LAT,
            NavigationContract.Route.Columns.LON,
            NavigationContract.Route.Columns.REVISION
    };

    public static final class Columns {
      public static final String SEQ = "seq";

      public static final String LAT = "lat";
      public static final String LON = "lon";

      public static final String REVISION = "revision";
    }
  }

  public static final class Live {
    public static final String[] ALL_COLUMNS = {
            NavigationContract.Live.Columns.SESSION_STATE,
            NavigationContract.Live.Columns.CAR_DIRECTION,
            NavigationContract.Live.Columns.PEDESTRIAN_DIRECTION,
            NavigationContract.Live.Columns.DIST_TO_TURN,
            NavigationContract.Live.Columns.DIST_TO_TARGET,
            NavigationContract.Live.Columns.DIST_TO_NEXT_STOP,
            NavigationContract.Live.Columns.TOTAL_TIME_SECONDS,
            NavigationContract.Live.Columns.TIME_TO_NEXT_STOP,
            NavigationContract.Live.Columns.CURRENT_STREET,
            NavigationContract.Live.Columns.NEXT_STREET,
            NavigationContract.Live.Columns.COMPLETION_PERCENT,
            NavigationContract.Live.Columns.EXIT_NUM,
            NavigationContract.Live.Columns.ROUTE_REVISION,
            NavigationContract.Live.Columns.ROUTE_POINT_COUNT,
            NavigationContract.Live.Columns.DEST_LAT,
            NavigationContract.Live.Columns.DEST_LON,
            NavigationContract.Live.Columns.DEST_TITLE
    };

    public static final class Columns {
      public static final String SESSION_STATE = "session_state";

      public static final String CAR_DIRECTION = "car_direction";
      public static final String PEDESTRIAN_DIRECTION = "pedestrian_direction";

      public static final String DIST_TO_TURN = "dist_to_turn";
      public static final String DIST_TO_TARGET = "dist_to_target";
      public static final String DIST_TO_NEXT_STOP = "dist_to_next_stop";

      public static final String TOTAL_TIME_SECONDS = "total_time_seconds";
      public static final String TIME_TO_NEXT_STOP = "time_to_next_stop";

      public static final String CURRENT_STREET = "current_street";
      public static final String NEXT_STREET = "next_street";

      public static final String COMPLETION_PERCENT = "completion_percent";

      public static final String EXIT_NUM = "exit_num";

      public static final String ROUTE_REVISION = "route_revision";
      public static final String ROUTE_POINT_COUNT = "route_point_count";

      public static final String DEST_LAT = "dest_lat";
      public static final String DEST_LON = "dest_lon";
      public static final String DEST_TITLE = "dest_title";
    }
  }
}
