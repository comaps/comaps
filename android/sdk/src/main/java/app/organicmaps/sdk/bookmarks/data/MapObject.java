package app.organicmaps.sdk.bookmarks.data;

import android.net.Uri;
import android.os.Parcel;
import android.text.TextUtils;

import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.os.ParcelCompat;

import app.organicmaps.sdk.routing.RoutePointInfo;
import app.organicmaps.sdk.search.Popularity;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.widget.placepage.PlacePageData;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.time.DateTimeException;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

// TODO(yunikkk): Refactor. Displayed information is different from edited information, and it's better to
// separate them. Simple getters from jni place_page::Info and osm::EditableFeature should be enough.
// Used from JNI.
@Keep
public class MapObject implements PlacePageData
{
  private static final String TAG = MapObject.class.getSimpleName();

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({POI, API_POINT, BOOKMARK, MY_POSITION, SEARCH, TRACK})
  public @interface MapObjectType
  {}

  public static final int POI = 0;
  public static final int API_POINT = 1;
  public static final int BOOKMARK = 2;
  public static final int MY_POSITION = 3;
  public static final int SEARCH = 4;
  public static final int TRACK = 5;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({OPENING_MODE_PREVIEW, OPENING_MODE_PREVIEW_PLUS, OPENING_MODE_DETAILS, OPENING_MODE_FULL})
  public @interface OpeningMode
  {}

  public static final int OPENING_MODE_PREVIEW = 0;
  public static final int OPENING_MODE_PREVIEW_PLUS = 1;
  public static final int OPENING_MODE_DETAILS = 2;
  public static final int OPENING_MODE_FULL = 3;

  private static final String kHttp = "http://";
  private static final String kHttps = "https://";

  @NonNull
  private final FeatureId mFeatureId;
  @MapObjectType
  private final int mMapObjectType;

  private String mTitle;
  @Nullable
  private final String mSecondaryTitle;
  private final String mSubtitle;
  private double mLat;
  private double mLon;
  private final String mAddress;
  @NonNull
  private final Metadata mMetadata;
  private final String mApiId;
  private final RoutePointInfo mRoutePointInfo;
  @OpeningMode
  private final int mOpeningMode;
  //  @NonNull
  //  private final Popularity mPopularity;
  @NonNull
  private final RoadWarningMarkType mRoadWarningMarkType;
  @Nullable
  private final Float mStarRating;
  /**
   * The total number of reviews of this object. We cannot rely on mReviews.size(), because
   * the mReviews might not contain all reviews.
   */
  private final int mReviewCount;
  /**
   * The most recent reviews of this object.
   */
  @NonNull
  private ArrayList<Review> mReviews;
  /**
   * The name of the app used to add/edit reviews.
   */
  @Nullable
  private final String mReviewEditorAppName;
  @NonNull
  private String mWikiArticle;
  @NonNull
  private final String mOsmDescription;
  @Nullable
  private List<String> mRawTypes;

  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
                   @Nullable String secondaryTitle, String subtitle, String address, double lat, double lon,
                   String apiId, @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode,
                   Popularity popularity, @Nullable Float starRating,
                   int reviewCount, @Nullable String reviewEditorAppName, @NonNull String wikiArticle, @NonNull String osmDescription,
                   int roadWarningType, @Nullable String[] rawTypes)
  {
    this(featureId, mapObjectType, title, secondaryTitle, subtitle, address, lat, lon, new Metadata(), apiId,
        routePointInfo, openingMode, popularity, starRating, reviewCount, reviewEditorAppName, wikiArticle, osmDescription, roadWarningType, rawTypes);
  }

  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
                   @Nullable String secondaryTitle, String subtitle, String address, double lat, double lon,
                   Metadata metadata, String apiId, @Nullable RoutePointInfo routePointInfo,
                   @OpeningMode int openingMode, Popularity popularity, @Nullable Float starRating,
                   int reviewCount, @Nullable String reviewEditorAppName, @NonNull String wikiArticle, @NonNull String osmDescription, int roadWarningType,
                   @Nullable String[] rawTypes)
  {
    mFeatureId = featureId;
    mMapObjectType = mapObjectType;
    mTitle = title;
    mSecondaryTitle = secondaryTitle;
    mSubtitle = subtitle;
    mAddress = address;
    mLat = lat;
    mLon = lon;
    mMetadata = metadata != null ? metadata : new Metadata();
    mApiId = apiId;
    mRoutePointInfo = routePointInfo;
    mOpeningMode = openingMode;
    // mPopularity = popularity;
    mStarRating = starRating;
    mReviewCount = reviewCount;
    mReviewEditorAppName = reviewEditorAppName;
    mReviews = new ArrayList<>();
    mWikiArticle = wikiArticle;
    mOsmDescription = osmDescription;
    mRoadWarningMarkType = RoadWarningMarkType.values()[roadWarningType];
    if (rawTypes != null)
      mRawTypes = new ArrayList<>(Arrays.asList(rawTypes));
  }

  protected MapObject(@MapObjectType int type, Parcel source)
  {
    this(Objects.requireNonNull(
            ParcelCompat.readParcelable(source, FeatureId.class.getClassLoader(), FeatureId.class)), // FeatureId
        type, // MapObjectType
        source.readString(), // Title
        source.readString(), // SecondaryTitle
        source.readString(), // Subtitle
        source.readString(), // Address
        source.readDouble(), // Lat
        source.readDouble(), // Lon
        ParcelCompat.readParcelable(source, Metadata.class.getClassLoader(), Metadata.class),
        source.readString(), // ApiId;
        ParcelCompat.readParcelable(source, RoutePointInfo.class.getClassLoader(),
            RoutePointInfo.class), // RoutePointInfo
        source.readInt(), // mOpeningMode
        Objects.requireNonNull(
            ParcelCompat.readParcelable(source, Popularity.class.getClassLoader(), Popularity.class)),
        (Float) source.readValue(Float.class.getClassLoader()), // mStarRating
        source.readInt(), // mReviewCount
        source.readString(), // mReviewEditorAppName
        Objects.requireNonNull(source.readString()),
        source.readString(),
        source.readInt(),
        null // mRawTypes
    );

    mRawTypes = readRawTypes(source);
    mReviews = readReviews(source);
  }

  @NonNull
  private ArrayList<Review> readReviews(@NonNull Parcel source)
  {
    // use the deprecated method because the replacement is not available at the min API level of this project
    @SuppressWarnings("deprecation") Review[] reviewArray = (Review[]) source.readParcelableArray(Review.class.getClassLoader());
    return new ArrayList<>(Arrays.asList(Objects.requireNonNull(reviewArray)));
  }

  @NonNull
  public static MapObject createMapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType,
                                          @NonNull String title, @NonNull String subtitle, double lat, double lon)
  {
    return new MapObject(featureId, mapObjectType, title, "", subtitle, "", lat, lon, null, "", null,
        OPENING_MODE_PREVIEW, Popularity.defaultInstance(), null, 0, null, "", null,
        RoadWarningMarkType.UNKNOWN.ordinal(), new String[0]);
  }

  @NonNull
  private static List<String> readRawTypes(@NonNull Parcel source)
  {
    List<String> types = new ArrayList<>();
    source.readStringList(types);
    return types;
  }

  /**
   * If you override {@link #equals(Object)} it is also required to override {@link #hashCode()}.
   * MapObject does not participate in any sets or other collections that need {@code hashCode()}.
   * So {@code sameAs()} serves as {@code equals()} but does not break the equals+hashCode contract.
   */
  public boolean sameAs(@Nullable MapObject other)
  {
    if (other == null)
      return false;

    if (this == other)
      return true;

    // noinspection SimplifiableIfStatement
    if (getClass() != other.getClass())
      return false;

    if (mFeatureId.isRealId() && other.getFeatureId().isRealId())
      return mFeatureId.equals(other.getFeatureId());

    return Double.doubleToLongBits(mLon) == Double.doubleToLongBits(other.mLon)
        && Double.doubleToLongBits(mLat) == Double.doubleToLongBits(other.mLat);
  }

  public static boolean same(@Nullable MapObject one, @Nullable MapObject another)
  {
    // noinspection SimplifiableIfStatement
    if (one == null && another == null)
      return true;

    return (one != null && one.sameAs(another));
  }

  public double getScale()
  {
    return 0;
  }

  @NonNull
  public String getTitle()
  {
    return mTitle;
  }

  public void setTitle(@NonNull String title)
  {
    mTitle = title;
  }

  @NonNull
  public String getName()
  {
    return getTitle();
  }

  @Nullable
  public String getSecondaryTitle()
  {
    return mSecondaryTitle;
  }

  @NonNull
  public String getSubtitle()
  {
    return mSubtitle;
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  @NonNull
  public String getAddress()
  {
    return mAddress;
  }

  @Nullable
  public Float getStarRating()
  {
    return mStarRating;
  }

  public int getReviewCount()
  {
    return mReviewCount;
  }

  public ArrayList<Review> getReviews()
  {
    return mReviews;
  }

  @Nullable
  public String getReviewEditorAppName()
  {
    return mReviewEditorAppName;
  }

  @NonNull
  public String getWikiArticle()
  {
    return mWikiArticle;
  }

  public void setWikiArticle(@NonNull String wikiArticle)
  {
    mWikiArticle = wikiArticle;
  }

  @NonNull
  public String getOsmDescription()
  {
    return mOsmDescription;
  }

  @NonNull
  public RoadWarningMarkType getRoadWarningMarkType()
  {
    return mRoadWarningMarkType;
  }

  @NonNull
  public String getMetadata(Metadata.MetadataType type)
  {
    final String res = mMetadata.getMetadata(type);
    return res == null ? "" : res;
  }

  @NonNull
  public String getWebsiteUrl(boolean strip, @NonNull Metadata.MetadataType type)
  {
    final String website = Uri.decode(getMetadata(type));
    final int len = website.length();
    if (strip && len > 1)
    {
      final int start = website.startsWith(kHttps) ? kHttps.length() : (website.startsWith(kHttp) ? kHttp.length() : 0);
      final int end = website.endsWith("/") ? len - 1 : len;
      return website.substring(start, end);
    }
    return website;
  }

  public String getApiId()
  {
    return mApiId;
  }

  public void setLat(double lat)
  {
    mLat = lat;
  }

  public void setLon(double lon)
  {
    mLon = lon;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void addMetadata(int type, String value)
  {
    mMetadata.addMetadata(type, value);
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void addReview(float starRating, int year, int month, int dayOfMonth, @NonNull String opinion, @NonNull String author)
  {
    try
    {
      mReviews.add(new Review(starRating, LocalDate.of(year, month, dayOfMonth), opinion, author));
    } catch (DateTimeException ex) {
      Logger.w(TAG, "invalid review date", ex);
    }
  }

  public boolean hasChargeSockets()
  {
    return !TextUtils.isEmpty(getMetadata(Metadata.MetadataType.FMD_CHARGE_SOCKETS));
  }

  public boolean hasPhoneNumber()
  {
    return !TextUtils.isEmpty(getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
  }

  public boolean hasAtm()
  {
    return mRawTypes.contains("amenity-atm");
  }

  public String getOrganic()
  {
    return getFeatureType("organic-");
  }

  public final boolean isMyPosition()
  {
    return mMapObjectType == MY_POSITION;
  }

  public final boolean isBookmark()
  {
    return mMapObjectType == BOOKMARK;
  }

  public final boolean isTrack()
  {
    return mMapObjectType == TRACK;
  }

  private String getFeatureType(String match)
  {
    for (String type : mRawTypes)
    {
      if (type.startsWith(match))
      {
        return type;
      }
    }
    return null;
  }

  @Nullable
  public RoutePointInfo getRoutePointInfo()
  {
    return mRoutePointInfo;
  }

  @OpeningMode
  public int getOpeningMode()
  {
    return mOpeningMode;
  }

  @NonNull
  public FeatureId getFeatureId()
  {
    return mFeatureId;
  }

  private static MapObject readFromParcel(Parcel source)
  {
    @MapObjectType
    int type = source.readInt();
    if (type == BOOKMARK)
      return new Bookmark(type, source);

    return new MapObject(type, source);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    // A map object type must be written first, since it's used in readParcel method to distinguish
    // what type of object should be read from the parcel.
    dest.writeInt(mMapObjectType);
    dest.writeParcelable(mFeatureId, 0);
    dest.writeString(mTitle);
    dest.writeString(mSecondaryTitle);
    dest.writeString(mSubtitle);
    dest.writeString(mAddress);
    dest.writeDouble(mLat);
    dest.writeDouble(mLon);
    dest.writeParcelable(mMetadata, 0);
    dest.writeString(mApiId);
    dest.writeParcelable(mRoutePointInfo, 0);
    dest.writeInt(mOpeningMode);
    // dest.writeParcelable(mPopularity, 0);
    dest.writeValue(mStarRating);
    dest.writeInt(mReviewCount);
    dest.writeString(mReviewEditorAppName);
    dest.writeString(mWikiArticle);
    dest.writeString(mOsmDescription);
    dest.writeInt(getRoadWarningMarkType().ordinal());
    // All collections are deserialized AFTER non-collection and primitive type objects,
    // so collections must be always serialized at the end.
    dest.writeStringList(mRawTypes);
    dest.writeParcelableArray(mReviews.toArray(new Review[0]), 0);
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;

    MapObject mapObject = (MapObject) o;
    return mFeatureId.equals(mapObject.mFeatureId);
  }

  @Override
  public int hashCode()
  {
    return mFeatureId.hashCode();
  }

  public static final Creator<MapObject> CREATOR = new Creator<>()
  {
    @Override
    public MapObject createFromParcel(Parcel source)
    {
      return readFromParcel(source);
    }

    @Override
    public MapObject[] newArray(int size)
    {
      return new MapObject[size];
    }
  };
}
