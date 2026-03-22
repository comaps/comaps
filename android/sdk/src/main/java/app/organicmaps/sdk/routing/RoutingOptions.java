package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.settings.BorderAvoidanceMode;
import app.organicmaps.sdk.settings.RoadType;
import java.util.HashSet;
import java.util.Set;

public final class RoutingOptions
{
  public static void addOption(@NonNull RoadType roadType)
  {
    nativeAddOption(roadType.ordinal());
  }

  public static void removeOption(@NonNull RoadType roadType)
  {
    nativeRemoveOption(roadType.ordinal());
  }

  public static boolean hasOption(@NonNull RoadType roadType)
  {
    return nativeHasOption(roadType.ordinal());
  }

  public static boolean hasAnyOptions()
  {
    if (getBorderAvoidanceMode() != BorderAvoidanceMode.None)
      return true;
    for (RoadType each : RoadType.values())
    {
      if (hasOption(each))
        return true;
    }
    return false;
  }

  @NonNull
  public static Set<RoadType> getActiveRoadTypes()
  {
    Set<RoadType> roadTypes = new HashSet<>();
    for (RoadType each : RoadType.values())
    {
      if (hasOption(each))
        roadTypes.add(each);
    }
    return roadTypes;
  }

  @NonNull
  public static BorderAvoidanceMode getBorderAvoidanceMode()
  {
    int mode = nativeGetBorderAvoidanceMode();
    if (mode >= 0 && mode < BorderAvoidanceMode.values().length)
      return BorderAvoidanceMode.values()[mode];
    return BorderAvoidanceMode.None;
  }

  public static void setBorderAvoidanceMode(@NonNull BorderAvoidanceMode mode)
  {
    nativeSetBorderAvoidanceMode(mode.ordinal());
  }

  @NonNull
  public static Set<String> getAvoidedBorderCountries()
  {
    String[] countries = nativeGetAvoidedBorderCountries();
    Set<String> result = new HashSet<>();
    if (countries != null)
    {
      for (String c : countries)
        result.add(c);
    }
    return result;
  }

  public static void setAvoidedBorderCountries(@NonNull Set<String> countries)
  {
    nativeSetAvoidedBorderCountries(countries.toArray(new String[0]));
  }

  @NonNull
  public static Set<String> getTopLevelCountries()
  {
    String[] countries = nativeGetTopLevelCountries();
    Set<String> result = new HashSet<>();
    if (countries != null)
    {
      for (String c : countries)
        result.add(c);
    }
    return result;
  }

  private RoutingOptions() throws IllegalAccessException
  {
    throw new IllegalAccessException("RoutingOptions is a utility class and should not be instantiated");
  }
  private static native void nativeAddOption(int option);

  private static native void nativeRemoveOption(int option);

  private static native boolean nativeHasOption(int option);

  private static native int nativeGetBorderAvoidanceMode();

  private static native void nativeSetBorderAvoidanceMode(int mode);

  private static native String[] nativeGetAvoidedBorderCountries();

  private static native void nativeSetAvoidedBorderCountries(String[] countries);

  private static native String[] nativeGetTopLevelCountries();
}
