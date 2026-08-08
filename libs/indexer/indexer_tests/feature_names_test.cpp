#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/feature_utils.hpp"

#include "i18n/string_utf8_multilang.hpp"

#include <string>

namespace feature_names_test
{
using namespace std;

using StrUtf8 = StringUtf8Multilang;

/* TODO: Not working right now, because the language is determined more dynamically. Needed to find a way to still test this without adding extra complexity.
 
void GetPreferredNames(feature::RegionData const & regionData, StrUtf8 const & src, std::string & primary, std::string & secondary)
{
  auto const translatedName = localisation::TranslatedFeatureName(src, regionData.GetLanguages());
  
  std::optional<std::string> const translatedPrimaryName = translatedName.m_primary;
  if (translatedPrimaryName.has_value())
    primary = translatedPrimaryName.value();
  
  std::optional<std::string> const translatedSecondaryName = translatedName.m_secondary;
  if (translatedSecondaryName.has_value())
    secondary = translatedSecondaryName.value();
}

UNIT_TEST(GetPrefferedNames)
{
  feature::RegionData regionData;
  std::vector<localisation::LanguageCode> const languageCodes = {"de", "ko"};
  regionData.SetLanguages(languageCodes);

  int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");
  string primary, secondary;
  bool const allowTranslit = false;

  {
    StrUtf8 src;
    src.AddString("fr", "fr name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "same name");
    src.AddString("en", "en name");
    src.AddString("ru", "same name");
    src.AddString("int_name", "int name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "same name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("de", "de name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "ko name", ());
  }

  {
    StrUtf8 src;
    src.AddString("en", "en name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "en name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("be");
    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "int name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("be");
    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("be");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }
}

UNIT_TEST(GetPrefferedNamesLocal)
{
  string primary, secondary;
  bool const allowTranslit = true;
  {
    feature::RegionData regionData;
    std::vector<localisation::LanguageCode> const languageCodes = {"de", "ko"};
    regionData.SetLanguages(languageCodes);

    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");

    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "default name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    std::vector<localisation::LanguageCode> const languageCodes = {"de", "ko"};
    regionData.SetLanguages(languageCodes);

    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("be");

    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "ru name", ());
    TEST_EQUAL(secondary, "", ());
  }
  {
    feature::RegionData regionData;
    std::vector<localisation::LanguageCode> const languageCodes = {"de", "ko"};
    regionData.SetLanguages(languageCodes);

    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");

    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetPreferredNames(regionData, src, deviceLang, allowTranslit, primary, secondary);

    TEST_EQUAL(primary, "int name", ());
    TEST_EQUAL(secondary, "", ());
  }
}

void GetReadableName(feature::RegionData const & regionData, StrUtf8 const & src, std::string & name)
{
  std::optional<std::string> const translatedName = localisation::TranslatedFeatureName(src, regionData.GetLanguages()).m_primary;
  if (translatedName.has_value())
    name = translatedName.value();
}

UNIT_TEST(GetReadableName)
{
  feature::RegionData regionData;
  std::vector<localisation::LanguageCode> const languageCodes = {"de", "ko"};
  regionData.SetLanguages(languageCodes);

  int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");
  bool const allowTranslit = false;
  string name;

  {
    StrUtf8 src;
    src.AddString("fr", "fr name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ko name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "en name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "int name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");
    src.AddString("ru", "ru name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ru name", ());
  }

  deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("de");

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "de name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("default", "default name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");
    src.AddString("int_name", "int name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "int name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "en name", ());
  }

  {
    StrUtf8 src;
    src.AddString("ko", "ko name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ko name", ());
  }
  {
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("be");
    StrUtf8 src;
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "ru name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("be");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("ru", "ru name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("int_name", "int name");
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "default name", ());
  }
  {
    feature::RegionData regionData;
    regionData.SetLanguages({"ru"});
    int8_t deviceLang = localisation::ConvertLanguageCodeToLanguageIndex("ru");
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("be", "be name");

    GetReadableName(regionData, src, deviceLang, allowTranslit, name);

    TEST_EQUAL(name, "en name", ());
  }
}
*/

/*
UNIT_TEST(GetNameForSearchOnBooking)
{
  {
    StrUtf8 src;
    feature::RegionData regionData;
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, localisation::kUnsupportedLanguageIndex, ());
    TEST(result.empty(), ());
  }
  {
    StrUtf8 src;
    src.AddString("default", "default name");
    feature::RegionData regionData;
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::kDefaultCode, ());
    TEST_EQUAL(result, "default name", ());
  }
  {
    StrUtf8 src;
    src.AddString("default", "default name");
    src.AddString("ko", "ko name");
    src.AddString("en", "en name");
    feature::RegionData regionData;
    regionData.SetLanguages({"ko", "en"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, StrUtf8::kDefaultCode, ());
    TEST_EQUAL(result, "default name", ());
  }
  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");
    feature::RegionData regionData;
    regionData.SetLanguages({"ko"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, localisation::ConvertLanguageCodeToLanguageIndex("ko"), ());
    TEST_EQUAL(result, "ko name", ());
  }
  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");
    src.AddString("de", "de name");
    feature::RegionData regionData;
    regionData.SetLanguages({"de", "ko"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, localisation::ConvertLanguageCodeToLanguageIndex("de"), ());
    TEST_EQUAL(result, "de name", ());
  }
  {
    StrUtf8 src;
    src.AddString("en", "en name");
    src.AddString("ko", "ko name");
    feature::RegionData regionData;
    regionData.SetLanguages({"de", "fr"});
    string result;
    auto lang = feature::GetNameForSearchOnBooking(regionData, src, result);
    TEST_EQUAL(lang, localisation::ConvertLanguageCodeToLanguageIndex("en"), ());
    TEST_EQUAL(result, "en name", ());
  }
}
*/
}  // namespace feature_names_test
