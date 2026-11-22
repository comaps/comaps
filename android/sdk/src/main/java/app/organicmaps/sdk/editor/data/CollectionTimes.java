package app.organicmaps.sdk.editor.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class CollectionTimes
{
  // java.util.Calendar day numbers starting from Monday.
  @NonNull
  public final int[] weekdays;
  @NonNull
  public final HoursMinutes[] times;

  public CollectionTimes(@NonNull int[] weekdays, @NonNull HoursMinutes[] times)
  {
    this.weekdays = weekdays;
    this.times = times;
  }

  public boolean containsWeekday(@IntRange(from = 1, to = 7) int day)
  {
    for (int d : weekdays)
      if (d == day)
        return true;
    return false;
  }
}
