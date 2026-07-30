package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public final class RouteGeometry
{
  @NonNull
  public static final RouteGeometry EMPTY = new RouteGeometry(0, new double[0], Double.NaN, Double.NaN, null);

  public final int mRevision;

  @NonNull
  public final double[] mLatLon;

  public final double mDestLat;
  public final double mDestLon;
  @Nullable
  public final String mDestTitle;

  private RouteGeometry(int revision, @NonNull double[] latLon, double destLat, double destLon,
                        @Nullable String destTitle)
  {
    mRevision = revision;
    mLatLon = latLon;
    mDestLat = destLat;
    mDestLon = destLon;
    mDestTitle = destTitle;
  }

  public int getPointCount()
  {
    return mLatLon.length / 2;
  }

  public boolean hasDestination()
  {
    return !Double.isNaN(mDestLat) && !Double.isNaN(mDestLon);
  }

  @NonNull
  public static RouteGeometry empty(int revision)
  {
    return new RouteGeometry(revision, new double[0], Double.NaN, Double.NaN, null);
  }

  @NonNull
  public static RouteGeometry from(int revision, @Nullable JunctionInfo[] junctions,
                                   @Nullable RouteMarkData[] routePoints)
  {
    final double[] latLon;
    if (junctions == null || junctions.length == 0)
    {
      latLon = new double[0];
    }
    else
    {
      latLon = new double[junctions.length * 2];
      for (int i = 0; i < junctions.length; ++i)
      {
        latLon[i * 2] = junctions[i].mLat;
        latLon[i * 2 + 1] = junctions[i].mLon;
      }
    }

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

    return new RouteGeometry(revision, latLon, destLat, destLon, destTitle);
  }
}
