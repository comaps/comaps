package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;

/**
 * Structured road information
 */
@Keep
@SuppressWarnings("unused")
public final class RoadInfo
{
  @NonNull
  public final String formattedName;
  @Nullable
  public final RoadShieldInfo shields;
  @NonNull
  public final String name;
  @NonNull
  public final String ref;
  @NonNull
  public final String junctionRef;
  @NonNull
  public final String destinationRef;
  @NonNull
  public final String destination;
  public final boolean isLink;

  private RoadInfo(@NonNull String formattedName, @Nullable RoadShieldInfo shields, @NonNull String name,
                   @NonNull String ref, @NonNull String junctionRef, @NonNull String destinationRef,
                   @NonNull String destination, boolean isLink)
  {
    this.formattedName = formattedName;
    this.shields = shields;
    this.name = name;
    this.ref = ref;
    this.junctionRef = junctionRef;
    this.destinationRef = destinationRef;
    this.destination = destination;
    this.isLink = isLink;
  }
}
