package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.util.Distance;

/**
 * Represents routing::RouteStepInfo class
 */
@Keep
@SuppressWarnings("unused")
public final class RouteStepInfo
{
    public final int index;
    public final int turnDirection;
    public final int pedestrianDirection;
    @Nullable
    public final String fromStreetName;
    @Nullable
    public final String toStreetName;
    public final int exitNum;
    public final double distMeters;
    public final Distance formattedDistance;
    public final String textualInstruction;

    private RouteStepInfo(int index, int turnDirection, int pedestrianDirection, @Nullable String fromStreetName,
                          @Nullable String toStreetName, int exitNum, double distMeters, @Nullable Distance formattedDistance,
                          String textualInstruction)
    {
        this.index = index;
        this.turnDirection = turnDirection;
        this.pedestrianDirection = pedestrianDirection;
        this.fromStreetName = fromStreetName;
        this.toStreetName = toStreetName;
        this.exitNum = exitNum;
        this.distMeters = distMeters;
        this.formattedDistance = formattedDistance;
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
