package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class RouteGeometryTest
{
  private static final double DELTA = 1e-9;

  private static RouteMarkData mark(RouteMarkType type, String title, double lat, double lon)
  {
    return new RouteMarkData(title, null /* subtitle */, type, 0 /* intermediateIndex */, true /* isVisible */,
                             false /* isMyPosition */, false /* isPassed */, lat, lon);
  }

  @Test
  public void packsJunctionsInterleaved()
  {
    final JunctionInfo[] junctions = {new JunctionInfo(59.437, 24.753), new JunctionInfo(59.438, 24.755)};

    final RouteGeometry geometry = RouteGeometry.from(1, junctions, null);

    assertEquals(2, geometry.getPointCount());
    assertArrayEquals(new double[] {59.437, 24.753, 59.438, 24.755}, geometry.mLatLon, DELTA);
  }

  @Test
  public void anEmptyJunctionArrayIsNoPoints()
  {
    final RouteGeometry geometry = RouteGeometry.from(1, new JunctionInfo[0], null);

    assertEquals(0, geometry.getPointCount());
    assertEquals(0, geometry.mLatLon.length);
  }

  @Test
  public void takesTheFinishAsTheDestination()
  {
    final RouteMarkData[] points = {mark(RouteMarkType.Start, "Here", 1, 2),
                                    mark(RouteMarkType.Intermediate, "On the way", 3, 4),
                                    mark(RouteMarkType.Finish, "There", 5, 6)};

    final RouteGeometry geometry = RouteGeometry.from(1, null, points);

    assertTrue(geometry.hasDestination());
    assertEquals(5, geometry.mDestLat, DELTA);
    assertEquals(6, geometry.mDestLon, DELTA);
    assertEquals("There", geometry.mDestTitle);
  }

  @Test
  public void hasNoDestinationWithoutAFinishMark()
  {
    final RouteMarkData[] points = {mark(RouteMarkType.Start, "Here", 1, 2)};

    final RouteGeometry geometry = RouteGeometry.from(1, null, points);

    assertFalse(geometry.hasDestination());
    assertNull(geometry.mDestTitle);
  }

  @Test
  public void toleratesNullsFromTheNativeSide()
  {
    final RouteGeometry geometry = RouteGeometry.from(7, null, null);

    assertEquals(7, geometry.mRevision);
    assertEquals(0, geometry.getPointCount());
    assertFalse(geometry.hasDestination());
  }

  @Test
  public void emptyKeepsTheRevisionSoItNeverGoesBackwards()
  {
    final RouteGeometry closed = RouteGeometry.empty(4);

    assertEquals(4, closed.mRevision);
    assertEquals(0, closed.getPointCount());
    assertFalse(closed.hasDestination());
  }

  @Test
  public void theSharedEmptyIsEmpty()
  {
    assertEquals(0, RouteGeometry.EMPTY.getPointCount());
    assertEquals(0, RouteGeometry.EMPTY.mRevision);
    assertFalse(RouteGeometry.EMPTY.hasDestination());
  }
}
