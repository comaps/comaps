package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;
import app.organicmaps.sdk.util.Distance;

/**
 * Represents routing::RouteStepInfo class
 */
@Keep
@SuppressWarnings("unused")
public final class RouteStepInfo
{
    public final int index;
    public final int sequence;
    public final int turnDirection;
    public final int pedestrianDirection;
    @Nullable
    public final String fromStreetName;
    @Nullable
    public final String toStreetName;
    @NonNull
    public final String toFormattedRoadName;
    @NonNull
    public final String toRef;
    @NonNull
    public final String toJunctionRef;
    @NonNull
    public final String toDestinationRef;
    @NonNull
    public final String toDestination;
    public final boolean toIsLink;
    public final int exitNum;
    public final double distMeters;
    public final double distFromStartMeters;
    public final Distance formattedDistance;
    @Nullable
    public final LaneInfo[] lanes;
    @Nullable
    public final RoadShieldInfo toRoadShields;
    public final String textualInstruction;

    private RouteStepInfo(int index, int sequence, int turnDirection, int pedestrianDirection,
                          @Nullable String fromStreetName, @Nullable String toStreetName,
                          @NonNull String toFormattedRoadName, @NonNull String toRef,
                          @NonNull String toJunctionRef, @NonNull String toDestinationRef,
                          @NonNull String toDestination, boolean toIsLink, int exitNum, double distMeters,
                          double distFromStartMeters, @Nullable Distance formattedDistance,
                          @Nullable LaneInfo[] lanes, @Nullable RoadShieldInfo toRoadShields,
                          String textualInstruction)
    {
        this.index = index;
        this.sequence = sequence;
        this.turnDirection = turnDirection;
        this.pedestrianDirection = pedestrianDirection;
        this.fromStreetName = fromStreetName;
        this.toStreetName = toStreetName;
        this.toFormattedRoadName = toFormattedRoadName;
        this.toRef = toRef;
        this.toJunctionRef = toJunctionRef;
        this.toDestinationRef = toDestinationRef;
        this.toDestination = toDestination;
        this.toIsLink = toIsLink;
        this.exitNum = exitNum;
        this.distMeters = distMeters;
        this.distFromStartMeters = distFromStartMeters;
        this.formattedDistance = formattedDistance;
        this.lanes = lanes;
        this.toRoadShields = toRoadShields;
        this.textualInstruction = textualInstruction;
    }

    public int getTurnDrawableRes()
    {
        if (turnDirection > 0 && turnDirection < CarDirection.values().length)
            return CarDirection.values()[turnDirection].getTurnRes();

        if (pedestrianDirection > 0 && pedestrianDirection < PedestrianTurnDirection.values().length)
            return PedestrianTurnDirection.values()[pedestrianDirection].getTurnRes();

        return 0;
    }

}
