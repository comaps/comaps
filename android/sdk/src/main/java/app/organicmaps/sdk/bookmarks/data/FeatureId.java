package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import java.util.Objects;

// Used by JNI.
@Keep
public class FeatureId implements Parcelable
{
  public static final Creator<FeatureId> CREATOR = new Creator<>() {
    @Override
    public FeatureId createFromParcel(Parcel in)
    {
      return new FeatureId(in);
    }

    @Override
    public FeatureId[] newArray(int size)
    {
      return new FeatureId[size];
    }
  };

  @NonNull
  public static final FeatureId EMPTY = new FeatureId("", -1L, 0);

  @NonNull
  private final String mMwmName;
  private final long mMwmVersion;
  private final int mFeatureIndex;

  public FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
  {
    mMwmName = mwmName;
    mMwmVersion = mwmVersion;
    mFeatureIndex = featureIndex;
  }

  private FeatureId(Parcel in)
  {
    mMwmName = Objects.requireNonNull(in.readString());
    mMwmVersion = in.readLong();
    mFeatureIndex = in.readInt();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mMwmName);
    dest.writeLong(mMwmVersion);
    dest.writeInt(mFeatureIndex);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @NonNull
  public String getMwmName()
  {
    return mMwmName;
  }

  public boolean isRealId()
  {
    return !TextUtils.isEmpty(mMwmName) && mMwmVersion >= 0 && mFeatureIndex > 0;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;

    FeatureId featureId = (FeatureId) o;

    if (mMwmVersion != featureId.mMwmVersion)
      return false;
    if (mFeatureIndex != featureId.mFeatureIndex)
      return false;
    return mMwmName.equals(featureId.mMwmName);
  }

  @Override
  public int hashCode()
  {
    int result = mMwmName.hashCode();
    result = 31 * result + Long.hashCode(mMwmVersion);
    result = 31 * result + mFeatureIndex;
    return result;
  }

  @NonNull
  @Override
  public String toString()
  {
    return "FeatureId{"
  + "mMwmName='" + mMwmName + '\'' + ", mMwmVersion=" + mMwmVersion + ", mFeatureIndex=" + mFeatureIndex + '}';
  }
}
