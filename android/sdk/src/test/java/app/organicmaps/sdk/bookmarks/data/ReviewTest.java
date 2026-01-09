package app.organicmaps.sdk.bookmarks.data;

import static org.junit.Assert.assertEquals;

import android.os.Parcel;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import java.time.LocalDate;

@RunWith(RobolectricTestRunner.class)
public final class ReviewTest
{
  @Test
  public void parcelableRoundTrip()
  {
    Review original = new Review(3.7, LocalDate.of(2026, 7, 27), "test review body", "Test Author");
    Parcel parcel = Parcel.obtain();
    try
    {
      parcel.writeParcelable(original, 0);
      parcel.setDataPosition(0);
      @SuppressWarnings("deprecation") Review restored = parcel.readParcelable(Review.class.getClassLoader());
      assertEquals(original, restored);
    } finally
    {
      parcel.recycle();
    }
  }

}
