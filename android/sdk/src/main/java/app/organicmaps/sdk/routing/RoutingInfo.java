package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;
import app.organicmaps.sdk.util.Distance;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class RoutingInfo
{
  public enum RoutingSessionState
  {
    // Same values as in enum class SessionState in "libs/routing/routing_callbacks.hpp".
    NoValidRoute(0),
    RouteBuilding(1),
    RouteNotStarted(2),
    OnRoute(3),
    RouteNeedsRebuild(4),
    RouteFinished(5),
    RouteNoFollowing(6),
    RouteRebuilding(7),
    OffRoute(8);

    final int mState;

    RoutingSessionState(int state)
    {
      mState = state;
    }

    public static RoutingSessionState fromIntValue(int value)
    {
      return switch (value)
      {
        case 1 -> RouteBuilding;
        case 2 -> RouteNotStarted;
        case 3 -> OnRoute;
        case 4 -> RouteNeedsRebuild;
        case 5 -> RouteFinished;
        case 6 -> RouteNoFollowing;
        case 7 -> RouteRebuilding;
        case 8 -> OffRoute;
        default -> NoValidRoute;
      };
    }

    public static boolean isNavigable(RoutingSessionState state)
    {
      return switch (state)
      {
        case RouteNotStarted, OnRoute, RouteFinished, OffRoute -> true;
        default -> false;
      };
    }
  }

  // Distance to target (end point of route).
  public final Distance distToTarget;
  // Next turn.
  public final Distance distToTurn;
  // Time in seconds to target (end point of route).
  public final int totalTimeInSeconds;
  // Current street name.
  public final String currentStreet;
  // The next street name.
  public final String nextStreet;
  @Nullable
  public final RoadShieldInfo nextStreetRoadShields;
  // The next to the next street name.
  public final String nextNextStreet;
  @Nullable
  public final RoadShieldInfo nextNextStreetRoadShields;
  public final double completionPercent;
  // For vehicle routing.
  public final CarDirection carDirection;
  public final CarDirection nextCarDirection;
  public final int exitNum;
  @Nullable
  public final LaneInfo[] lanes;
  // For pedestrian routing.
  public final PedestrianTurnDirection pedestrianTurnDirection;
  // Current speed limit in meters per second.
  // If no info about speed limit then speedLimitMps < 0.
  public final double speedLimitMps;
  private final boolean speedCamLimitExceeded;
  private final boolean shouldPlayWarningSignal;
  // Routing session state.
  public final RoutingSessionState routingSessionState;
  // Index of the next intermediate stop:
  //  -1 = invalid next intermediate stops.
  //   0 = there are no next intermediate stops.
  //   1 = intermediate stop #1.
  //   2 = intermediate stop #2.
  //   and so on...
  public final int indexOfNextStop;
  // Time & distance information to the next intermediate stop.
  public final Distance distToNextStop;
  public final int timeToNextStop;

  private RoutingInfo(Distance distToTarget, Distance distToTurn, String currentStreet, String nextStreet,
                      @Nullable RoadShieldInfo nextStreetRoadShields, String nextNextStreet,
                      @Nullable RoadShieldInfo nextNextStreetRoadShields, double completionPercent, int vehicleTurnOrdinal,
                      int vehicleNextTurnOrdinal, int pedestrianTurnOrdinal, int exitNum, int totalTime,
                      @Nullable LaneInfo[] lanes, double speedLimitMps, boolean speedLimitExceeded,
                      boolean shouldPlayWarningSignal, int routingSessionState,
                      int indexOfNextStop, Distance distToNextStop, int timeToNextStop)
  {
    this.distToTarget = distToTarget;
    this.distToTurn = distToTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.nextStreetRoadShields = nextStreetRoadShields;
    this.nextNextStreet = nextNextStreet;
    this.nextNextStreetRoadShields = nextNextStreetRoadShields;
    this.totalTimeInSeconds = totalTime;
    this.completionPercent = completionPercent;
    this.carDirection = CarDirection.values()[vehicleTurnOrdinal];
    this.nextCarDirection = CarDirection.values()[vehicleNextTurnOrdinal];
    this.lanes = lanes;
    this.exitNum = exitNum;
    this.pedestrianTurnDirection = PedestrianTurnDirection.values()[pedestrianTurnOrdinal];
    this.speedLimitMps = speedLimitMps;
    this.speedCamLimitExceeded = speedLimitExceeded;
    this.shouldPlayWarningSignal = shouldPlayWarningSignal;
    this.routingSessionState = RoutingSessionState.fromIntValue(routingSessionState);
    this.indexOfNextStop = indexOfNextStop;
    this.distToNextStop = distToNextStop;
    this.timeToNextStop = timeToNextStop;
  }

  public boolean isSpeedCamLimitExceeded()
  {
    return speedCamLimitExceeded;
  }

  public boolean shouldPlayWarningSignal()
  {
    return shouldPlayWarningSignal;
  }
}
