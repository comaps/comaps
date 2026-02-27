package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
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
    RouteRebuilding(7);

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
        default -> NoValidRoute;
      };
    }

    public static boolean isNavigable(RoutingSessionState state)
    {
      return switch (state)
      {
        case RouteNotStarted, OnRoute, RouteFinished -> true;
        default -> false;
      };
    }
  }

  // Target (end point of route).
  public final Distance distToTarget;
  // Next turn.
  public final Distance distToTurn;

  public final int totalTimeInSeconds;
  // Current street name.
  public final String currentStreet;
  // The next street name.
  public final String nextStreet;
  // The next next street name.
  public final String nextNextStreet;
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

  private RoutingInfo(Distance distToTarget, Distance distToTurn, String currentStreet, String nextStreet,
                      String nextNextStreet, double completionPercent, int vehicleTurnOrdinal,
                      int vehicleNextTurnOrdinal, int pedestrianTurnOrdinal, int exitNum, int totalTime,
                      @Nullable LaneInfo[] lanes, double speedLimitMps, boolean speedLimitExceeded,
                      boolean shouldPlayWarningSignal, int routingSessionState)
  {
    this.distToTarget = distToTarget;
    this.distToTurn = distToTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.nextNextStreet = nextNextStreet;
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
