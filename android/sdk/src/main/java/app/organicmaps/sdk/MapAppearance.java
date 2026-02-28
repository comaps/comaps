package app.organicmaps.sdk;

import androidx.annotation.NonNull;

public enum MapAppearance
{
  Light(0),
  Dark(1);

  MapAppearance(int value)
  {
    this.value = value;
  }

  @NonNull
  public static MapAppearance get()
  {
    return valueOf(nativeGet());
  }

  public static void set(@NonNull MapAppearance mapAppearance)
  {
    nativeSet(mapAppearance.value);
  }

  @NonNull
  public static MapAppearance valueOf(int value)
  {
    for (MapAppearance mapAppearance : MapAppearance.values())
    {
      if (mapAppearance.value == value)
        return mapAppearance;
    }
    throw new IllegalArgumentException("Unknown map appearancde value: " + value);
  }

  private final int value;

  private static native void nativeSet(int mapAppearance);

  private static native int nativeGet();
}
