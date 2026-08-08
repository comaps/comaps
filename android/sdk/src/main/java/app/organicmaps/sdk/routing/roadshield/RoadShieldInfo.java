package app.organicmaps.sdk.routing.roadshield;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoutingInfo;

/// Information about road shields that are displayed in street name fields of {@link RoutingInfo}.
public final class RoadShieldInfo
{
  /// An array of road shields for the target (next) street
  @Nullable
  public final RoadShield[] targetRoadShields;
  /// An array of road shields for the junction
  @Nullable
  public final RoadShield[] junctionRoadShields;

  // Used by JNI.
  @Keep
  private RoadShieldInfo(@Nullable RoadShield[] targetRoadShields, @Nullable RoadShield[] junctionRoadShields)
  {
    this.targetRoadShields = targetRoadShields;
    this.junctionRoadShields = junctionRoadShields;
  }

  public boolean hasTargetRoadShields()
  {
    return targetRoadShields != null && targetRoadShields.length > 0;
  }

  public boolean hasJunctionRoadShields()
  {
    return junctionRoadShields != null && junctionRoadShields.length > 0;
  }

  @Override
  @NonNull
  public String toString()
  {
    StringBuilder sb = new StringBuilder("RoadShieldInfo{targetRoadShields=");
    if (!hasTargetRoadShields())
      sb.append("null");
    else
    {
      sb.append("[");
      for (int i = 0; i < targetRoadShields.length; i++)
      {
        sb.append(targetRoadShields[i]);
        if (i != targetRoadShields.length - 1)
          sb.append(", ");
      }
      sb.append("]");
    }
    sb.append(", junctionRoadShields=");
    if (!hasJunctionRoadShields())
      sb.append("null");
    else
    {
      sb.append("[");
      for (int i = 0; i < junctionRoadShields.length; i++)
      {
        sb.append(junctionRoadShields[i]);
        if (i != junctionRoadShields.length - 1)
          sb.append(", ");
      }
      sb.append("]");
    }
    sb.append("}");
    return sb.toString();
  }
}
