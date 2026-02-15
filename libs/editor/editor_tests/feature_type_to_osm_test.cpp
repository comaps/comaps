#include "testing/testing.hpp"

#include "editor/feature_type_to_osm.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

using namespace editor;

UNIT_TEST(simpleType)
{
  std::string data =
      "amenity|restaurant;61;\n"
      "amenity|bicycle_parking;1071;\n";

  classificator::Load();

  TypeToOSMTranslator translator(false);
  std::stringstream s(data);
  translator.LoadFromStream(s);

  uint32_t type = classif().GetTypeByReadableObjectName("amenity-restaurant");
  std::vector<OsmElement::Tag> result = translator.OsmTagsFromType(type);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0].m_key, "amenity", ());
  TEST_EQUAL(result[0].m_value, "restaurant", ());
}

UNIT_TEST(simpleTypeWithTags)
{
  std::string data =
      "building;[building];;addr:housenumber;name;1;\n"
      "amenity|school;[amenity=school],[education=school];;name;int_name;36;\n"
      "amenity|doctors;[amenity=doctors][healthcare=doctor],[amenity=doctors],[healthcare=doctor];;name;int_name;207;\n";

  classificator::Load();

  TypeToOSMTranslator translator(false);
  std::stringstream s(data);
  translator.LoadFromStream(s);

  uint32_t buildingType = classif().GetTypeByReadableObjectName("building");
  std::vector<OsmElement::Tag> buildingResult = translator.OsmTagsFromType(buildingType);
  TEST_EQUAL(buildingResult.size(), 1, ());
  TEST_EQUAL(buildingResult[0].m_key, "building", ());
  TEST_EQUAL(buildingResult[0].m_value, "yes", ());

  uint32_t schoolType = classif().GetTypeByReadableObjectName("amenity-school");
  std::vector<OsmElement::Tag> schoolResult = translator.OsmTagsFromType(schoolType);
  TEST_EQUAL(schoolResult.size(), 1, ());
  TEST_EQUAL(schoolResult[0].m_key, "amenity", ());
  TEST_EQUAL(schoolResult[0].m_value, "school", ());

  uint32_t doctorType = classif().GetTypeByReadableObjectName("amenity-doctors");
  std::vector<OsmElement::Tag> doctorResult = translator.OsmTagsFromType(doctorType);
  TEST_EQUAL(doctorResult.size(), 2, ());
  TEST_EQUAL(doctorResult[0].m_key, "amenity", ());
  TEST_EQUAL(doctorResult[0].m_value, "doctors", ());
  TEST_EQUAL(doctorResult[1].m_key, "healthcare", ());
  TEST_EQUAL(doctorResult[1].m_value, "doctor", ());
}

UNIT_TEST(complexType)
{
  std::string data =
      "building;[building];;addr:housenumber;name;1;\n"
      " # comment that should be ignored\n"
      "\n"
      "amenity|restaurant;61;\n"
      "tourism|information|office;[tourism=information][information=office];;name;int_name;313;\n"
      "historic|castle|fortress;[historic=castle][castle_type=fortress],[historic=fortress];;name;int_name;1144;\n"
      "#comment\n"
      "amenity|place_of_worship|christian|mormon;[amenity=place_of_worship][religion=christian][denomination=mormon];;name;int_name;1572;\n";

  classificator::Load();

  TypeToOSMTranslator translator(false);
  std::stringstream s(data);
  translator.LoadFromStream(s);

  uint32_t officeType = classif().GetTypeByReadableObjectName("tourism-information-office");
  std::vector<OsmElement::Tag> officeResult = translator.OsmTagsFromType(officeType);
  TEST_EQUAL(officeResult.size(), 2, ());
  TEST_EQUAL(officeResult[0].m_key, "tourism", ());
  TEST_EQUAL(officeResult[0].m_value, "information", ());
  TEST_EQUAL(officeResult[1].m_key, "information", ());
  TEST_EQUAL(officeResult[1].m_value, "office", ());

  uint32_t fortressType = classif().GetTypeByReadableObjectName("historic-castle-fortress");
  std::vector<OsmElement::Tag> fortressResult = translator.OsmTagsFromType(fortressType);
  TEST_EQUAL(fortressResult.size(), 2, ());
  TEST_EQUAL(fortressResult[0].m_key, "historic", ());
  TEST_EQUAL(fortressResult[0].m_value, "castle", ());
  TEST_EQUAL(fortressResult[1].m_key, "castle_type", ());
  TEST_EQUAL(fortressResult[1].m_value, "fortress", ());

  uint32_t mormonType = classif().GetTypeByReadableObjectName("amenity-place_of_worship-christian-mormon");
  std::vector<OsmElement::Tag> mormonResult = translator.OsmTagsFromType(mormonType);
  TEST_EQUAL(mormonResult.size(), 3, ());
  TEST_EQUAL(mormonResult[0].m_key, "amenity", ());
  TEST_EQUAL(mormonResult[0].m_value, "place_of_worship", ());
  TEST_EQUAL(mormonResult[1].m_key, "religion", ());
  TEST_EQUAL(mormonResult[1].m_value, "christian", ());
  TEST_EQUAL(mormonResult[2].m_key, "denomination", ());
  TEST_EQUAL(mormonResult[2].m_value, "mormon", ());
}

UNIT_TEST(mandatorySelector)
{
  std::string data =
      "amenity|parking|fee;[amenity=parking][fee];;name;int_name;125;\n"
      "highway|track|bridge;[highway=track][bridge?];;name;int_name;193;\n"
      "shop;[shop?];;name;int_name;943;\n"
      "disusedbusiness;[disused:shop?],[disused:amenity=restaurant],[disused:amenity=fast_food],[disused:amenity=cafe],[disused:amenity=pub],[disused:amenity=bar];;;;1237;\n";

  classificator::Load();

  TypeToOSMTranslator translator(false);
  std::stringstream s(data);
  translator.LoadFromStream(s);

  uint32_t parkingType = classif().GetTypeByReadableObjectName("amenity-parking-fee");
  std::vector<OsmElement::Tag> parkingResult = translator.OsmTagsFromType(parkingType);
  TEST_EQUAL(parkingResult.size(), 2, ());
  TEST_EQUAL(parkingResult[0].m_key, "amenity", ());
  TEST_EQUAL(parkingResult[0].m_value, "parking", ());
  TEST_EQUAL(parkingResult[1].m_key, "fee", ());
  TEST_EQUAL(parkingResult[1].m_value, "yes", ());

  uint32_t trackType = classif().GetTypeByReadableObjectName("highway-track-bridge");
  std::vector<OsmElement::Tag> trackResult = translator.OsmTagsFromType(trackType);
  TEST_EQUAL(trackResult.size(), 2, ());
  TEST_EQUAL(trackResult[0].m_key, "highway", ());
  TEST_EQUAL(trackResult[0].m_value, "track", ());
  TEST_EQUAL(trackResult[1].m_key, "bridge", ());
  TEST_EQUAL(trackResult[1].m_value, "yes", ());

  uint32_t shopType = classif().GetTypeByReadableObjectName("shop");
  std::vector<OsmElement::Tag> shopResult = translator.OsmTagsFromType(shopType);
  TEST_EQUAL(shopResult.size(), 1, ());
  TEST_EQUAL(shopResult[0].m_key, "shop", ());
  TEST_EQUAL(shopResult[0].m_value, "yes", ());

  uint32_t disusedType = classif().GetTypeByReadableObjectName("disusedbusiness");
  std::vector<OsmElement::Tag> disusedResult = translator.OsmTagsFromType(disusedType);
  TEST_EQUAL(disusedResult.size(), 1, ());
  TEST_EQUAL(disusedResult[0].m_key, "disused:shop", ());
  TEST_EQUAL(disusedResult[0].m_value, "yes", ());
}

UNIT_TEST(forbiddenSelector)
{
  std::string data =
      "amenity|lounger;[amenity=lounger][!seasonal];;name;int_name;153;\n"
      "amenity|charging_station|motorcar|small;[amenity=charging_station][motorcar?][!capacity],[amenity=charging_station][motorcar?][capacity=1],[amenity=charging_station][motorcar?][capacity=2];;name;int_name;201;\n";

  classificator::Load();

  TypeToOSMTranslator translator(false);
  std::stringstream s(data);
  translator.LoadFromStream(s);

  uint32_t loungerType = classif().GetTypeByReadableObjectName("amenity-lounger");
  std::vector<OsmElement::Tag> loungerResult = translator.OsmTagsFromType(loungerType);
  TEST_EQUAL(loungerResult.size(), 1, ());
  TEST_EQUAL(loungerResult[0].m_key, "amenity", ());
  TEST_EQUAL(loungerResult[0].m_value, "lounger", ());

  uint32_t chargingType = classif().GetTypeByReadableObjectName("amenity-charging_station-motorcar-small");
  std::vector<OsmElement::Tag> chargingResult = translator.OsmTagsFromType(chargingType);
  TEST_EQUAL(chargingResult.size(), 2, ());
  TEST_EQUAL(chargingResult[0].m_key, "amenity", ());
  TEST_EQUAL(chargingResult[0].m_value, "charging_station", ());
  TEST_EQUAL(chargingResult[1].m_key, "motorcar", ());
  TEST_EQUAL(chargingResult[1].m_value, "yes", ());
}

UNIT_TEST(ignoreComments)
{
  std::string data =
      "building;[building];;addr:housenumber;name;1;\n"
      " # comment that should be ignored\n"
      "\n"
      "deprecated:waterway|riverbank:05.2024;52;x\n"
      "amenity|restaurant;61;\n"
      "moved:amenity|telephone:05.2024;122;amenity|telephone\n"
      "natural|lake;564;natural|water|lake\n";  // moved type, should be ignored

  classificator::Load();

  TypeToOSMTranslator translator(false);
  std::stringstream s(data);
  translator.LoadFromStream(s);
}

UNIT_TEST(loadConfigFile)
{
  TypeToOSMTranslator translator(false);
  translator.LoadConfigFile();

  size_t size = translator.GetStorage().size();
  LOG(LINFO, ("Size of feature type storage:", size));
  ASSERT(size > 1300, ());
  ASSERT(size < 1700, ());
}

UNIT_TEST(testWithRealFile)
{
  classificator::Load();

  uint32_t restaurantType = classif().GetTypeByReadableObjectName("amenity-restaurant");
  std::vector<OsmElement::Tag> restaurantResult = GetOSMTranslator().OsmTagsFromType(restaurantType);
  TEST_EQUAL(restaurantResult.size(), 1, ());
  TEST_EQUAL(restaurantResult[0].m_key, "amenity", ());
  TEST_EQUAL(restaurantResult[0].m_value, "restaurant", ());

  uint32_t officeType = classif().GetTypeByReadableObjectName("tourism-information-office");
  std::vector<OsmElement::Tag> officeResult = GetOSMTranslator().OsmTagsFromType(officeType);
  TEST_EQUAL(officeResult.size(), 2, ());
  TEST_EQUAL(officeResult[0].m_key, "tourism", ());
  TEST_EQUAL(officeResult[0].m_value, "information", ());
  TEST_EQUAL(officeResult[1].m_key, "information", ());
  TEST_EQUAL(officeResult[1].m_value, "office", ());
}
