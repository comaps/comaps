package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * A versioned route plan
 */
@Keep
@SuppressWarnings("unused")
public final class RoutePlan
{
  public final long routeId;

  @NonNull
  public final RouteStepInfo[] steps;

  private RoutePlan(long routeId, @NonNull RouteStepInfo[] steps)
  {
    this.routeId = routeId;
    this.steps = steps;
  }
}
