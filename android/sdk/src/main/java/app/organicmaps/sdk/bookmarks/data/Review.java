package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;

import java.time.LocalDate;
import java.util.Objects;

public record Review(float starRating, @NonNull LocalDate date, @NonNull String opinion,
                     @NonNull String author) implements Parcelable
{
  public static final Creator<Review> CREATOR = new Creator<>()
  {
    @Override
    public Review createFromParcel(Parcel source)
    {
      return readFromParcel(source);
    }

    @Override
    public Review[] newArray(int size)
    {
      return new Review[size];
    }
  };

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeFloat(starRating);
    dest.writeLong(date.toEpochDay());
    dest.writeString(opinion);
    dest.writeString(author);
  }

  private static Review readFromParcel(@NonNull Parcel source)
  {
    float starRating = source.readFloat();
    LocalDate date = LocalDate.ofEpochDay(source.readLong());
    String opinion = Objects.requireNonNull(source.readString());
    String author = Objects.requireNonNull(source.readString());
    return new Review(starRating, date, opinion, author);
  }
}
