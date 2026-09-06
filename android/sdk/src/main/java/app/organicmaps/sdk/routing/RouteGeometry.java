package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public final class RouteGeometry
{
  private static final JunctionInfo[] NO_JUNCTIONS = new JunctionInfo[0];

  @NonNull
  public static final RouteGeometry EMPTY = new RouteGeometry(0, NO_JUNCTIONS, Double.NaN, Double.NaN, null);

  public final int mRevision;

  @NonNull
  public final JunctionInfo[] mJunctions;

  public final double mDestLat;
  public final double mDestLon;
  @Nullable
  public final String mDestTitle;

  private RouteGeometry(int revision, @NonNull JunctionInfo[] junctions, double destLat, double destLon,
                        @Nullable String destTitle)
  {
    mRevision = revision;
    mJunctions = junctions;
    mDestLat = destLat;
    mDestLon = destLon;
    mDestTitle = destTitle;
  }

  public int getPointCount()
  {
    return mJunctions.length;
  }

  public boolean hasDestination()
  {
    return !Double.isNaN(mDestLat) && !Double.isNaN(mDestLon);
  }

  @NonNull
  public static RouteGeometry empty(int revision)
  {
    return new RouteGeometry(revision, NO_JUNCTIONS, Double.NaN, Double.NaN, null);
  }

  @NonNull
  public static RouteGeometry from(int revision, @Nullable JunctionInfo[] junctions,
                                   @Nullable RouteMarkData[] routePoints)
  {
    double destLat = Double.NaN;
    double destLon = Double.NaN;
    String destTitle = null;
    if (routePoints != null)
    {
      for (final RouteMarkData point : routePoints)
      {
        if (point != null && point.mPointType == RouteMarkType.Finish)
        {
          destLat = point.mLat;
          destLon = point.mLon;
          destTitle = point.mTitle;
          break;
        }
      }
    }

    return new RouteGeometry(revision, junctions != null ? junctions : NO_JUNCTIONS, destLat, destLon, destTitle);
  }
}
