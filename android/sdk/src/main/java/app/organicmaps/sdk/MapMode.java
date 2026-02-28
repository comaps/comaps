package app.organicmaps.sdk;

import androidx.annotation.NonNull;

public enum MapMode
{
  Walking(0),
  Cycling(1),
  Driving(2),
  PublicTransport(3);

  MapMode(int value)
  {
    this.value = value;
  }

  @NonNull
  public static MapMode get()
  {
    return valueOf(nativeGet());
  }

  public static void set(@NonNull MapMode mapMode)
  {
    nativeSet(mapMode.value);
  }

  @NonNull
  public static MapMode valueOf(int value)
  {
    for (MapMode mapMode : MapMode.values())
    {
      if (mapMode.value == value)
        return mapMode;
    }
    throw new IllegalArgumentException("Unknown map mode value: " + value);
  }

  private final int value;

  private static native void nativeSet(int mapMode);

  private static native int nativeGet();
}
