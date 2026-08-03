#include "editor/review.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include "coding/url.hpp"

#include "editor/osm_feature_lookup.hpp"
#include "editor/xml_feature.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "indexer/map_object.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace reviews
{

namespace internal
{

// An arbitrary value based on what MapComplete uses
constexpr int kPointUncertaintyMeters = 10;

uint Uncertainty(osm::MapObject const & mapObject)
{
  std::vector<m2::PointD> points;
  switch (mapObject.GetGeomType())
  {
  case feature::GeomType::Point: return kPointUncertaintyMeters;
  case feature::GeomType::Line: points = mapObject.GetPoints(); break;
  case feature::GeomType::Area: points = mapObject.GetTriangesAsPoints(); break;
  default: MYTHROW(ReviewEditorException, ("Unsupported geometry type", mapObject.GetGeomType()));
  }
  double maxDist = 0;
  auto const center = mapObject.GetMercator();
  for (auto p : points)
    maxDist = std::max(maxDist, mercator::DistanceOnEarth(center, p));
  return static_cast<uint>(maxDist);
}

[[nodiscard]] std::optional<std::string> Mangrove::EditorUrl(osm::MapObject const & mapObject)
{
  ms::LatLon const center = mapObject.GetLatLon();
  auto const name = std::string(mapObject.GetDefaultName());  // TODO: use primary type if name empty?
  uint uncertainty;
  try
  {
    uncertainty = Uncertainty(mapObject);
  }
  catch (ReviewEditorException const & ex)
  {
    LOG(LWARNING, ("Error calculating uncertainty:", ex.what()));
    return std::nullopt;
  }
  std::ostringstream geo;
  geo.precision(10);
  geo << "geo:" << center.m_lat << "," << center.m_lon << "?";
  if (!name.empty())
    geo << "q=" << url::UrlEncode(name) << "&";
  geo << "u=" << uncertainty;
  return "https://mangrove.reviews/search?sub=" + url::UrlEncode(geo.str());
}

[[nodiscard]] std::optional<std::string> MapCompleteTheme::EditorUrl(osm::MapObject const & mapObject) const
{
  try
  {
    editor::XMLFeature const feature = osm::OsmFeatureLookup().GetMatchingFeatureFromOSM(mapObject);
    return "https://mapcomplete.org/" + mThemeId + ".html#" + feature.GetTypeString() + "/" + feature.GetOSMIdString();
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Error looking up OSM entity for feature", mapObject.GetID(), ex.what()));
  }
  return std::nullopt;
}

using ReviewEditorSpec = std::variant<Mangrove, MapCompleteTheme>;

auto constexpr kMangrove = Mangrove{};
auto const kMapCompleteBicycleRental = MapCompleteTheme{"bicycle_rental"};
auto const kMapCompleteCafesAndPubs = MapCompleteTheme{"cafes_and_pubs"};
auto const kMapCompleteCinemas = MapCompleteTheme{"cinemas"};
auto const kMapCompleteFood = MapCompleteTheme{"food"};
auto const kMapCompleteHealthcare = MapCompleteTheme{"healthcare"};
auto const kMapCompleteHotels = MapCompleteTheme{"hotels"};
auto const kMapCompleteIcecream = MapCompleteTheme{"icecream"};
auto const kMapCompleteLibraries = MapCompleteTheme{"libraries"};
auto const kMapCompleteMuseums = MapCompleteTheme{"museums"};
auto const kMapCompletePlaygrounds = MapCompleteTheme{"playgrounds"};
auto const kMapCompleteShops = MapCompleteTheme{"shops"};
auto const kMapCompleteSports = MapCompleteTheme{"sports"};
auto const kMapCompleteToilets = MapCompleteTheme{"toilets"};

/// Until CoMaps has a built-in review editor, we direct the users
/// to existing web UIs for contributing reviews to Mangrove.
/// Mangrove has a generic review interface which can be used for any
/// feature, but lacks support for OSM-specific metadata. MapComplete has more
/// comprehensive OSM support, but requires that the feature being reviewed
/// is included in one of its themes. When a MapComplete theme exists for a
/// given feature we want to use it; otherwise we fall back to Mangrove.
/// There might also be some feature types where reviews do not make sense.
/// We exclude these from the review editor spec mapping.
std::map<std::string, ReviewEditorSpec> const reviewEditorConfigs = {
    {"amenity-bank", kMangrove},
    {"amenity-bar", kMapCompleteCafesAndPubs},
    {"amenity-bicycle_rental", kMapCompleteBicycleRental},
    {"amenity-biergarten", kMapCompleteCafesAndPubs},
    {"amenity-bureau_de_change", kMangrove},
    {"amenity-cafe", kMapCompleteCafesAndPubs},
    {"amenity-car_rental", kMangrove},
    {"amenity-car_wash", kMangrove},
    {"amenity-casino", kMangrove},
    {"amenity-childcare", kMangrove},
    {"amenity-cinema", kMapCompleteCinemas},
    {"amenity-clinic", kMapCompleteHealthcare},
    {"amenity-college", kMangrove},
    {"amenity-comunity_centre", kMangrove},
    {"amenity-community_centre-youth_centre", kMangrove},
    {"amenity-conference_centre", kMangrove},
    {"amenity-dentist", kMapCompleteHealthcare},
    {"amenity-doctors", kMapCompleteHealthcare},
    {"amenity-driving_school", kMangrove},
    {"amenity-events_venue", kMangrove},
    {"amenity-exhibition_centre", kMangrove},
    {"amenity-fast_food", kMapCompleteFood},
    {"amenity-flight_school", kMangrove},
    {"amenity-food_court", kMapCompleteFood},
    {"amenity-fuel", kMangrove},
    {"amenity-hospital", kMapCompleteHealthcare},
    {"amenity-ice_cream", kMapCompleteIcecream},
    {"amenity-internet_cafe", kMangrove},
    {"amenity-kindergarten", kMangrove},
    {"amenity-language_school", kMangrove},
    {"amenity-library", kMapCompleteLibraries},
    {"amenity-love_hotel", kMapCompleteHotels},
    {"amenity-marketplace", kMangrove},
    {"amenity-money_transfer", kMangrove},
    {"amenity-motorcycle_parking", kMangrove},
    {"amenity-motorcycle_rental", kMangrove},
    {"amenity-music_school", kMangrove},
    {"amenity-nightclub", kMapCompleteCafesAndPubs},
    {"amenity-nursing_home", kMangrove},
    {"amenity-parking", kMangrove},
    {"amenity-pharmacy", kMapCompleteHealthcare},
    {"amenity-planetarium", kMangrove},
    {"amenity-police", kMangrove},
    {"amenity-post_office", kMangrove},
    {"amenity-prep_school", kMangrove},
    {"amenity-pub", kMapCompleteCafesAndPubs},
    {"amenity-public_bath", kMangrove},
    {"amenity-recycling-centre", kMangrove},
    {"amenity-restaurant", kMapCompleteFood},
    {"amenity-sailing_school", kMangrove},
    {"amenity-school", kMangrove},
    {"amenity-social_facility", kMangrove},
    {"amenity-theatre", kMangrove},
    {"amenity-toilets", kMapCompleteToilets},
    {"amenity-townhall", kMangrove},
    {"amenity-university", kMangrove},
    {"amenity-vehicle_inspection", kMangrove},
    {"amenity-veterinary", kMangrove},
    {"craft-beekeeper", kMangrove},
    {"craft-blacksmith", kMangrove},
    {"craft-brewery", kMangrove},
    {"craft-carpenter", kMangrove},
    {"craft-caterer", kMangrove},
    {"craft-confectionery", kMangrove},
    {"craft-electrician", kMangrove},
    {"craft-electronics_repair", kMangrove},
    {"craft-gardener", kMangrove},
    {"craft-grinding_mill", kMangrove},
    {"craft-handicraft", kMangrove},
    {"craft-hvac", kMangrove},
    {"craft-key_cutter", kMapCompleteShops},
    {"craft-locksmith", kMangrove},
    {"craft-metal_construction", kMangrove},
    {"craft-painter", kMangrove},
    {"craft-photographer", kMangrove},
    {"craft-plumber", kMangrove},
    {"craft-sawmill", kMangrove},
    {"craft-shoemaker", kMangrove},
    {"craft-tailor", kMangrove},
    {"craft-winery", kMangrove},
    {"healthcare-physiotherapist", kMapCompleteHealthcare},
    {"healthcare-psychotherapist", kMapCompleteHealthcare},
    {"leisure-amusement_arcade", kMangrove},
    {"leisure-bowling_alley", kMangrove},
    {"leisure-dance", kMangrove},
    {"leisure-dog_park", kMangrove},
    {"leisure-escape_game", kMangrove},
    {"leisure-fitness_centre", kMapCompleteSports},
    {"leisure-fitness_centre-sport-yoga", kMapCompleteSports},
    {"leisure-fitness_station", kMapCompleteSports},
    {"leisure-garden", kMangrove},
    {"leisure-golf_course", kMangrove},
    {"leisure-hackerspace", kMangrove},
    {"leisure-indoor_play", kMangrove},
    {"leisure-marina", kMangrove},
    {"leisure-miniature_golf", kMangrove},
    {"leisure-nature_reserve", kMangrove},
    {"leisure-park", kMangrove},
    {"leisure-pitch", kMapCompleteSports},
    {"leisure-playground", kMapCompletePlaygrounds},
    {"leisure-resort", kMangrove},
    {"leisure-sauna", kMangrove},
    {"leisure-sports_centre", kMapCompleteSports},
    {"leisure-sports_centre-sport-swimming", kMapCompleteSports},
    {"leisure-sports_hall", kMangrove},
    {"leisure-stadium", kMangrove},
    {"leisure-swimming_pool", kMangrove},
    {"leisure-track", kMangrove},
    {"leisure-water_park", kMangrove},
    {"natural-beach", kMangrove},
    {"office-company", kMangrove},
    {"office-diplomatic", kMangrove},
    {"office-estate_agent", kMangrove},
    {"office-government", kMangrove},
    {"office-insurance", kMangrove},
    {"office-lawyer", kMangrove},
    {"office-ngo", kMangrove},
    {"office-security", kMangrove},
    {"office-telecommunication", kMangrove},
    {"shop-agrarian", kMapCompleteShops},
    {"shop-alcohol", kMapCompleteShops},
    {"shop-antiques", kMapCompleteShops},
    {"shop-appliance", kMapCompleteShops},
    {"shop-art", kMapCompleteShops},
    {"shop-auction", kMapCompleteShops},
    {"shop-baby_goods", kMapCompleteShops},
    {"shop-bag", kMapCompleteShops},
    {"shop-bakery", kMapCompleteShops},
    {"shop-bathroom_furnishing", kMapCompleteShops},
    {"shop-beauty", kMapCompleteShops},
    {"shop-beauty-day_spa", kMapCompleteShops},
    {"shop-beauty-nails", kMapCompleteShops},
    {"shop-bed", kMapCompleteShops},
    {"shop-beverages", kMapCompleteShops},
    {"shop-bicycle", kMapCompleteShops},
    {"shop-bookmaker", kMapCompleteShops},
    {"shop-books", kMapCompleteShops},
    {"shop-boutique", kMapCompleteShops},
    {"shop-butcher", kMapCompleteShops},
    {"shop-camera", kMapCompleteShops},
    {"shop-cannabis", kMapCompleteShops},
    {"shop-car_parts", kMapCompleteShops},
    {"shop-car_repair", kMapCompleteShops},
    {"shop-car_repair-tyres", kMapCompleteShops},
    {"shop-caravan", kMapCompleteShops},
    {"shop-carpet", kMapCompleteShops},
    {"shop-car", kMapCompleteShops},
    {"shop-charity", kMapCompleteShops},
    {"shop-cheese", kMapCompleteShops},
    {"shop-chemist", kMapCompleteShops},
    {"shop-chocolate", kMapCompleteShops},
    {"shop-clothes", kMapCompleteShops},
    {"shop-coffee", kMapCompleteShops},
    {"shop-collector", kMapCompleteShops},
    {"shop-computer", kMapCompleteShops},
    {"shop-confectionery", kMapCompleteShops},
    {"shop-convenience", kMapCompleteShops},
    {"shop-copyshop", kMapCompleteShops},
    {"shop-cosmetics", kMapCompleteShops},
    {"shop-craft", kMapCompleteShops},
    {"shop-curtain", kMapCompleteShops},
    {"shop-dairy", kMapCompleteShops},
    {"shop-deli", kMapCompleteShops},
    {"shop-department_store", kMapCompleteShops},
    {"shop-doityourself", kMapCompleteShops},
    {"shop-dry_cleaning", kMapCompleteShops},
    {"shop-electrical", kMapCompleteShops},
    {"shop-electronics", kMapCompleteShops},
    {"shop-erotic", kMapCompleteShops},
    {"shop-fabric", kMapCompleteShops},
    {"shop-farm", kMapCompleteShops},
    {"shop-fashion_accessories", kMapCompleteShops},
    {"shop-fishing", kMapCompleteShops},
    {"shop-florist", kMapCompleteShops},
    {"shop-funeral_directors", kMapCompleteShops},
    {"shop-furniture", kMapCompleteShops},
    {"shop-garden_centre", kMapCompleteShops},
    {"shop-gas", kMapCompleteShops},
    {"shop-gift", kMapCompleteShops},
    {"shop-greengrocer", kMapCompleteShops},
    {"shop-grocery", kMapCompleteShops},
    {"shop-hairdresser", kMapCompleteShops},
    {"shop-hardware", kMapCompleteShops},
    {"shop-health_food", kMapCompleteShops},
    {"shop-hearing_aids", kMapCompleteShops},
    {"shop-herbalist", kMapCompleteShops},
    {"shop-hifi", kMapCompleteShops},
    {"shop-houseware", kMapCompleteShops},
    {"shop-interior_decoration", kMapCompleteShops},
    {"shop-jewelry", kMapCompleteShops},
    {"shop-kiosk", kMapCompleteShops},
    {"shop-kitchen", kMapCompleteShops},
    {"shop-laundry", kMapCompleteShops},
    {"shop-lighting", kMapCompleteShops},
    {"shop-lottery", kMapCompleteShops},
    {"shop-mall", kMangrove},
    {"shop-massage", kMapCompleteShops},
    {"shop-medical_supply", kMapCompleteShops},
    {"shop-mobile_phone", kMapCompleteShops},
    {"shop-money_lender", kMapCompleteShops},
    {"shop-motorcycle_repair", kMapCompleteShops},
    {"shop-motorcycle", kMapCompleteShops},
    {"shop-musical_instrument", kMapCompleteShops},
    {"shop-music", kMapCompleteShops},
    {"shop-newsagent", kMapCompleteShops},
    {"shop-nutrition_supplements", kMapCompleteShops},
    {"shop-optician", kMapCompleteShops},
    {"shop-outdoor", kMapCompleteShops},
    {"shop-outpost", kMapCompleteShops},
    {"shop-paint", kMapCompleteShops},
    {"shop-pasta", kMapCompleteShops},
    {"shop-pawnbroker", kMapCompleteShops},
    {"shop-perfumery", kMapCompleteShops},
    {"shop-pet_grooming", kMapCompleteShops},
    {"shop-pet", kMapCompleteShops},
    {"shop-photo", kMapCompleteShops},
    {"shop-seafood", kMapCompleteShops},
    {"shop-second_hand", kMapCompleteShops},
    {"shop-sewing", kMapCompleteShops},
    {"shop-shoes", kMapCompleteShops},
    {"shop-sports", kMapCompleteShops},
    {"shop-stationery", kMapCompleteShops},
    {"shop-storage_rental", kMapCompleteShops},
    {"shop-supermarket", kMapCompleteShops},
    {"shop-tattoo", kMapCompleteShops},
    {"shop-tea", kMapCompleteShops},
    {"shop-telecommunication", kMapCompleteShops},
    {"shop-ticket", kMapCompleteShops},
    {"shop-tobacco", kMapCompleteShops},
    {"shop-toys", kMapCompleteShops},
    {"shop-trade", kMapCompleteShops},
    {"shop-travel_agency", kMapCompleteShops},
    {"shop-tyres", kMapCompleteShops},
    {"shop-variety_store", kMapCompleteShops},
    {"shop-video_games", kMapCompleteShops},
    {"shop-video", kMapCompleteShops},
    {"shop-watches", kMapCompleteShops},
    {"shop-water", kMapCompleteShops},
    {"shop-wholesale", kMapCompleteShops},
    {"shop-wine", kMapCompleteShops},
    {"tourism-alpine_hut", kMangrove},
    {"tourism-apartment", kMapCompleteHotels},
    {"tourism-aquarium", kMangrove},
    {"tourism-artwork", kMangrove},
    {"tourism-artwork-painting", kMangrove},
    {"tourism-artwork-sculpture", kMangrove},
    {"tourism-artwork-statue", kMangrove},
    {"tourism-attraction", kMangrove},
    {"tourism-camp_site", kMapCompleteHotels},
    {"tourism-caravan_site", kMangrove},
    {"tourism-chalet", kMapCompleteHotels},
    {"tourism-gallery", kMapCompleteMuseums},
    {"tourism-guest_house", kMapCompleteHotels},
    {"tourism-hostel", kMapCompleteHotels},
    {"tourism-hotel", kMapCompleteHotels},
    {"tourism-information", kMangrove},
    {"tourism-information-office", kMangrove},
    {"tourism-information-visitor_centre", kMangrove},
    {"tourism-motel", kMapCompleteHotels},
    {"tourism-museum", kMapCompleteMuseums},
    {"tourism-picnic_site", kMangrove},
    {"tourism-theme_park", kMangrove},
    {"tourism-viewpoint", kMangrove},
    {"tourism-wilderness_hut", kMangrove},
    {"tourism-zoo", kMangrove},
};

ReviewEditorSpec BetterSpec(std::optional<ReviewEditorSpec> const & optSpec1, ReviewEditorSpec const & spec2)
{
  // prefer MapComplete over Mangrove
  if (optSpec1.has_value() && std::holds_alternative<MapCompleteTheme>(optSpec1.value()))
    return optSpec1.value();
  return spec2;
}

std::optional<ReviewEditorSpec> SpecFor(osm::MapObject const & mapObject)
{
  std::optional<ReviewEditorSpec> spec;
  for (std::string const & rt : mapObject.GetTypes().ToObjectNames())
    if (auto found = reviewEditorConfigs.find(rt); found != reviewEditorConfigs.end())
      spec.emplace(BetterSpec(spec, found->second));
  return spec;
}

std::string AppName(ReviewEditorApp const & app)
{
  switch (app)
  {
  case ReviewEditorApp::Mangrove: return "Mangrove";
  case ReviewEditorApp::MapComplete: return "MapComplete";
  default: LOG(LWARNING, ("Unknown review editor app:", static_cast<int>(app))); return "[unknown]";
  }
}

}  // namespace internal

std::optional<std::string> GetReviewEditorApp(osm::MapObject const & mapObject)
{
  auto spec = internal::SpecFor(mapObject);
  if (!spec.has_value())
    return std::nullopt;
  return std::visit([](auto const & s) { return internal::AppName(s.EditorApp()); }, *spec);
}

std::optional<std::string> GetReviewEditorUrl(osm::MapObject const & mapObject)
{
  auto spec = internal::SpecFor(mapObject);
  if (!spec.has_value())
    return std::nullopt;
  return std::visit([&mapObject](auto const & s) { return s.EditorUrl(mapObject); }, *spec);
}

}  // namespace reviews
