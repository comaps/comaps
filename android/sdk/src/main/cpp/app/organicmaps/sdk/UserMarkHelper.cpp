#include "UserMarkHelper.hpp"

#include "app/organicmaps/sdk/routing/RoutePointInfo.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

#include "indexer/reviews_display.hpp"
#include "indexer/reviews_model.hpp"

#include "map/elevation_info.hpp"
#include "map/place_page_info.hpp"

#include "base/string_utils.hpp"

namespace usermark_helper
{

void InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, osm::MapObject const & src)
{
  static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
  ASSERT(addId, ());

  src.ForEachMetadataReadable([env, &mapObject](osm::MapObject::MetadataID id, std::string const & meta)
  {
    /// @todo Make separate processing of non-string values like FMD_DESCRIPTION.
    /// Actually, better to call separate getters instead of ToString processing.
    if (!meta.empty())
    {
      jni::TScopedLocalRef metaString(env, jni::ToJavaString(env, meta));
      env->CallVoidMethod(mapObject, addId, static_cast<jint>(id), metaString.get());
    }
  });
}

void InjectReviews(JNIEnv * env, jclass const clazz, jobject const mapObject, reviews::FeatureReviews const & reviews)
{
  // public void addReview(float starRating, int year, int month, int dayOfMonth, @NonNull String opinion, @NonNull
  // String author)
  static jmethodID const addId = env->GetMethodID(clazz, "addReview",
                                                  "("
                                                  "F"                   // starRating
                                                  "III"                 // year, month, dayOfMonth
                                                  "Ljava/lang/String;"  // opinion
                                                  "Ljava/lang/String;"  // author
                                                  ")V");
  ASSERT(addId, ());

  for (auto & r : reviews.reviews)
  {
    auto starRating = static_cast<jfloat>(reviews::ToStarRating(r.rating));
    auto year = static_cast<jint>(r.date.year());
    auto month = static_cast<jint>(static_cast<unsigned>(r.date.month()));
    auto dayOfMonth = static_cast<jint>(static_cast<unsigned>(r.date.day()));
    jni::TScopedLocalRef jOpinion(env, jni::ToJavaString(env, r.opinion));
    jni::TScopedLocalRef jAuthor(env, jni::ToJavaString(env, r.author));
    env->CallVoidMethod(mapObject, addId, starRating, year, month, dayOfMonth, jOpinion.get(), jAuthor.get());
  }
}

// jobject CreatePopularity(JNIEnv * env, place_page::Info const & info)
//{
//   static jclass const popularityClass =
//     jni::GetGlobalClassRef(env, "app/organicmaps/sdk/search/Popularity");
//   static jmethodID const popularityConstructor =
//     jni::GetConstructorID(env, popularityClass, "(I)V");
//   auto const popularityValue = info.GetPopularity();
//   return env->NewObject(popularityClass, popularityConstructor, static_cast<jint>(popularityValue));
// }

/// Holds common arguments required to construct MapObject and its subclasses.
struct MapObjectArgs
{
  jni::TScopedLocalRef jFeatureId;
  jni::TScopedLocalRef jTitle;
  jni::TScopedLocalRef jSecondaryTitle;
  jni::TScopedLocalRef jSubtitle;
  jni::TScopedLocalRef jAddress;
  jni::TScopedLocalRef jStarRating;
  jint reviewCount;
  jni::TScopedLocalRef jWikiDescription;
  jni::TScopedLocalRef jOsmDescription;

  /// Extract MapObject args from place_page::Info
  MapObjectArgs(JNIEnv * env, place_page::Info const & info)
    : jFeatureId(env, CreateFeatureId(env, info))
    , jTitle(env, jni::ToJavaString(env, info.GetTitle()))
    , jSecondaryTitle(env, jni::ToJavaString(env, info.GetSecondaryTitle()))
    , jSubtitle(env, jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSubtitle()))
    , jAddress(env, jni::ToJavaString(env, info.GetSecondarySubtitle()))
    , jStarRating(env, info.GetReviews()
                           .transform(
                               [&env](reviews::FeatureReviews const & r)
  {
    return jni::ToJavaBoxedFloat(env, reviews::ToStarRating(r.averageRating));
  }).value_or(nullptr))
    , reviewCount(static_cast<jint>(
          info.GetReviews().transform([](reviews::FeatureReviews const & r) { return r.reviews.size(); }).value_or(0)))
    , jWikiDescription(env, jni::ToJavaString(env, info.GetWikiDescription()))
    , jOsmDescription(env, jni::ToJavaString(env, info.GetOSMDescription()))
  {}

  static jobject CreateFeatureId(JNIEnv * env, place_page::Info const & info)
  {
    // public FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
    static jmethodID const featureCtorId = jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

    auto const fID = info.GetID();
    jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, fID.GetMwmName()));
    return env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(),
                                                        (jlong)fID.GetMwmVersion(), (jint)fID.m_index);
  }
};

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info, int mapObjectType, double lat, double lon,
                        bool parseMeta, bool parseApi, jobject const & routingPointInfo, jobject const & popularity,
                        jobjectArray jrawTypes)
{
  //  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
  //                   @Nullable String secondaryTitle, String subtitle, String address, double lat, double lon,
  //                   String apiId, @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode,
  //                   Popularity popularity, @Nullable Float starRating,
  //                   int reviewCount, @NonNull String wikiArticle, @NonNull String osmDescription,
  //                   int roadWarningType, @Nullable String[] rawTypes)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_mapObjectClazz,
                            "("
                            "Lapp/organicmaps/sdk/bookmarks/data/FeatureId;"  // featureId
                            "I"                                               // mapObjectType
                            "Ljava/lang/String;"                              // title
                            "Ljava/lang/String;"                              // secondaryTitle
                            "Ljava/lang/String;"                              // subtitle
                            "Ljava/lang/String;"                              // address
                            "DD"                                              // lat, lon
                            "Ljava/lang/String;"                              // appId
                            "Lapp/organicmaps/sdk/routing/RoutePointInfo;"    // routePointInfo
                            "I"                                               // openingMode
                            "Lapp/organicmaps/sdk/search/Popularity;"         // popularity
                            "Ljava/lang/Float;"                               // starRating
                            "I"                                               // reviewCount
                            "Ljava/lang/String;"                              // wikiArticle
                            "Ljava/lang/String;"                              // osmDescription
                            "I"                                               // roadWarnType
                            "[Ljava/lang/String;"                             // rawTypes
                            ")V");

  MapObjectArgs args(env, info);
  jni::TScopedLocalRef jApiId(env, jni::ToJavaString(env, parseApi ? info.GetApiUrl() : ""));

  jobject mapObject = env->NewObject(g_mapObjectClazz, ctorId, args.jFeatureId.get(), mapObjectType, args.jTitle.get(),
                                     args.jSecondaryTitle.get(), args.jSubtitle.get(), args.jAddress.get(), lat, lon,
                                     jApiId.get(), routingPointInfo, static_cast<jint>(info.GetOpeningMode()),
                                     popularity, args.jStarRating.get(), args.reviewCount, args.jWikiDescription.get(),
                                     args.jOsmDescription.get(), static_cast<jint>(info.GetRoadType()), jrawTypes);

  if (info.GetReviews().has_value())
    InjectReviews(env, g_mapObjectClazz, mapObject, info.GetReviews().value());
  if (parseMeta)
    InjectMetadata(env, g_mapObjectClazz, mapObject, info);
  return mapObject;
}

jobject CreateTrack(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                    jni::TScopedLocalRef const & routingPointInfo, jobject const & popularity)
{
  //  Track(@NonNull FeatureId featureId, @IntRange(from = 0) long categoryId, @IntRange(from = 0) long trackId,
  //        String title, @Nullable String secondaryTitle, @Nullable String subtitle, @Nullable String address,
  //        @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode, @NonNull Popularity popularity,
  //        @Nullable Float starRating, @IntRange(from = 0) int reviewCount,
  //        @NonNull String wikiArticle, @NonNull String osmDescription, @Nullable String[] rawTypes, int color,
  //        Distance length, double lat, double lon)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_trackClazz,
                            "("
                            "Lapp/organicmaps/sdk/bookmarks/data/FeatureId;"  // featureId
                            "J"                                               // categoryId
                            "J"                                               // trackId
                            "Ljava/lang/String;"                              // title
                            "Ljava/lang/String;"                              // secondaryTitle
                            "Ljava/lang/String;"                              // subtitle
                            "Ljava/lang/String;"                              // address
                            "Lapp/organicmaps/sdk/routing/RoutePointInfo;"    // routePointInfo
                            "I"                                               // openingMode
                            "Lapp/organicmaps/sdk/search/Popularity;"         // popularity
                            "Ljava/lang/Float;"                               // starRating
                            "I"                                               // reviewCount
                            "Ljava/lang/String;"                              // wikiArticle
                            "Ljava/lang/String;"                              // osmDescription
                            "[Ljava/lang/String;"                             // rawTypes
                            "I"                                               // color
                            "Lapp/organicmaps/sdk/util/Distance;"             // length
                            "DD"                                              // lat, lon
                            ")V");

  auto const trackId = info.GetTrackId();
  auto const track = frm()->GetBookmarkManager().GetTrack(trackId);
  jint androidColor = track->GetColor(0).GetARGB();
  auto const categoryId = track->GetGroupId();
  ms::LatLon const ll = info.GetLatLon();
  MapObjectArgs args(env, info);

  jobject mapObject = env->NewObject(
      g_trackClazz, ctorId, args.jFeatureId.get(), static_cast<jlong>(categoryId), static_cast<jlong>(trackId),
      args.jTitle.get(), args.jSecondaryTitle.get(), args.jSubtitle.get(), args.jAddress.get(), routingPointInfo.get(),
      info.GetOpeningMode(), popularity, args.jStarRating.get(), args.reviewCount, args.jWikiDescription.get(),
      args.jOsmDescription.get(), jrawTypes.get(), androidColor,
      ToJavaDistance(env, platform::Distance::CreateFormatted(track->GetLengthMeters())),
      static_cast<jdouble>(ll.m_lat), static_cast<jdouble>(ll.m_lon));

  if (info.GetReviews().has_value())
    InjectReviews(env, g_mapObjectClazz, mapObject, info.GetReviews().value());
  if (info.HasMetadata())
    InjectMetadata(env, g_mapObjectClazz, mapObject, info);
  return mapObject;
}

jobject CreateBookmark(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                       jni::TScopedLocalRef const & routingPointInfo, jobject const & popularity)
{
  //  public Bookmark(@NonNull FeatureId featureId, @IntRange(from = 0) long categoryId,
  //                  @IntRange(from = 0) long bookmarkId, String title, @Nullable String secondaryTitle,
  //                  @Nullable String subtitle, @Nullable String address, @Nullable RoutePointInfo routePointInfo,
  //                  @OpeningMode int openingMode, @NonNull Popularity popularity, @Nullable Float starRating, int
  //                  reviewCount, @NonNull String wikiArticle,
  //                  @NonNull String osmDescription, @Nullable String[] rawTypes)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_bookmarkClazz,
                            "("
                            "Lapp/organicmaps/sdk/bookmarks/data/FeatureId;"  // featureId
                            "J"                                               // categoryId
                            "J"                                               // bookmarkId
                            "Ljava/lang/String;"                              // title
                            "Ljava/lang/String;"                              // secondaryTitle
                            "Ljava/lang/String;"                              // subtitle
                            "Ljava/lang/String;"                              // address
                            "Lapp/organicmaps/sdk/routing/RoutePointInfo;"    // routePointInfo
                            "I"                                               // openingMode
                            "Lapp/organicmaps/sdk/search/Popularity;"         // popularity
                            "Ljava/lang/Float;"                               // starRating
                            "I"                                               // reviewCount
                            "Ljava/lang/String;"                              // wikiArticle
                            "Ljava/lang/String;"                              // osmDescription
                            "[Ljava/lang/String;"                             // rawTypes
                            ")V");

  auto const bookmarkId = info.GetBookmarkId();
  auto const categoryId = info.GetBookmarkCategoryId();
  MapObjectArgs args(env, info);

  jobject mapObject = env->NewObject(g_bookmarkClazz, ctorId, args.jFeatureId.get(), static_cast<jlong>(categoryId),
                                     static_cast<jlong>(bookmarkId), args.jTitle.get(), args.jSecondaryTitle.get(),
                                     args.jSubtitle.get(), args.jAddress.get(), routingPointInfo.get(),
                                     info.GetOpeningMode(), popularity, args.jStarRating.get(), args.reviewCount,
                                     args.jWikiDescription.get(), args.jOsmDescription.get(), jrawTypes.get());

  if (info.GetReviews().has_value())
    InjectReviews(env, g_mapObjectClazz, mapObject, info.GetReviews().value());
  if (info.HasMetadata())
    InjectMetadata(env, g_mapObjectClazz, mapObject, info);
  return mapObject;
}

jobject CreateElevationPoint(JNIEnv * env, ElevationInfo::Point const & point)
{
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ElevationInfo$Point");
  // public Point(double distance, int altitude, double latitude, double longitude)
  static jmethodID const pointCtorId = jni::GetConstructorID(env, pointClass, "(DIDD)V");
  return env->NewObject(
      pointClass, pointCtorId, static_cast<jdouble>(point.m_distance), static_cast<jint>(point.m_point.GetAltitude()),
      static_cast<jdouble>(point.m_point.GetPoint().x), static_cast<jdouble>(point.m_point.GetPoint().y));
}

jobjectArray ToElevationPointArray(JNIEnv * env, ElevationInfo::Points const & points)
{
  CHECK(!points.empty(), ("Elevation points must be non empty!"));
  static jclass const pointClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/ElevationInfo$Point");
  return jni::ToJavaArray(env, pointClass, points, [](JNIEnv * env, ElevationInfo::Point const & item)
  { return CreateElevationPoint(env, item); });
}

jobject CreateElevationInfo(JNIEnv * env, ElevationInfo const & info)
{
  // public ElevationInfo(@NonNull Point[] points, int difficulty);
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_elevationInfoClazz, "([Lapp/organicmaps/sdk/bookmarks/data/ElevationInfo$Point;I)V");

  jni::TScopedLocalObjectArrayRef jPoints(env, ToElevationPointArray(env, info.GetPoints()));
  return env->NewObject(g_elevationInfoClazz, ctorId, jPoints.get(), static_cast<jint>(info.GetDifficulty()));
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  jni::TScopedLocalObjectArrayRef jrawTypes(env, jni::ToJavaStringArray(env, info.GetRawTypes()));

  jni::TScopedLocalRef routingPointInfo(env, nullptr);
  if (info.IsRoutePoint())
    routingPointInfo.reset(CreateRoutePointInfo(env, info));

  // jni::TScopedLocalRef popularity(env, CreatePopularity(env, info));
  jobject popularity = nullptr;

  if (info.IsBookmark())
    return CreateBookmark(env, info, jrawTypes, routingPointInfo, popularity);

  ms::LatLon const ll = info.GetLatLon();
  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
  if (info.IsMyPosition())
  {
    return CreateMapObject(env, info, kMyPosition, ll.m_lat, ll.m_lon, false /* parseMeta */, false /* parseApi */,
                           routingPointInfo.get(), popularity, jrawTypes.get());
  }

  if (info.HasApiUrl())
  {
    return CreateMapObject(env, info, kApiPoint, ll.m_lat, ll.m_lon, true /* parseMeta */, true /* parseApi */,
                           routingPointInfo.get(), popularity, jrawTypes.get());
  }

  if (info.IsTrack())
    return CreateTrack(env, info, jrawTypes, routingPointInfo, popularity);

  return CreateMapObject(env, info, kPoi, ll.m_lat, ll.m_lon, true /* parseMeta */, false /* parseApi */,
                         routingPointInfo.get(), popularity, jrawTypes.get());
}

jobject CreateFeatureId(JNIEnv * env, FeatureID const & fid)
{
  static jmethodID const featureCtorId = jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

  auto const & info = fid.m_mwmId.GetInfo();
  jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, info ? info->GetCountryName() : ""));
  return env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(),
                        info ? static_cast<jlong>(info->GetVersion()) : 0, static_cast<jint>(fid.m_index));
}

jobjectArray ToFeatureIdArray(JNIEnv * env, std::vector<FeatureID> const & ids)
{
  if (ids.empty())
    return nullptr;

  return jni::ToJavaArray(env, g_featureIdClazz, ids,
                          [](JNIEnv * env, FeatureID const & fid) { return CreateFeatureId(env, fid); });
}
}  // namespace usermark_helper
