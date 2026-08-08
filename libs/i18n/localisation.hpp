#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace localisation
{
using namespace std;

typedef int8_t LanguageIndex;
typedef string LanguageCode;
typedef string TransliteratorId;
struct Language
{
  LanguageCode m_languageCode;

  /// Native language name
  string m_name;

  /// Transliterators to latin ids.
  vector<TransliteratorId> m_transliteratorsIds;
};

static LanguageIndex constexpr kUnsupportedLanguageIndex = -1;
static LanguageIndex constexpr kDefaultNameIndex = 0;
static LanguageIndex constexpr kEnglishLanguageIndex = 1;
static LanguageIndex constexpr kInternationalNameIndex = 7;
static LanguageIndex constexpr kAlternativeNameIndex = 53;
static LanguageIndex constexpr kOldNameIndex = 55;
static LanguageIndex constexpr kBelarusianLanguageIndex = 12;
static LanguageIndex constexpr kRussianLanguageIndex = 8;
static LanguageIndex constexpr kJapaneseLanguageIndex = 2;
static LanguageIndex constexpr kJapaneseKatakanaLanguageIndex = 45;
static LanguageIndex constexpr kJapaneseRomanizedLanguageIndex = 18;
static LanguageIndex constexpr kKoreanLanguageIndex = 14;
static LanguageIndex constexpr kKoreanRomanizedLanguageIndex = 4;
static LanguageIndex constexpr kChineseLanguageIndex = 10;
static LanguageIndex constexpr kChinesePinyinLanguageIndex = 22;
static int8_t constexpr kMaxSupportedLanguages = 64;
static LanguageCode constexpr kReservedLanguageCode = "reserved";
static string const kMapLanguageSetting = "MapLanguageCode";
static string const kAlternativeMapLanguageHandlingSetting = "MapLanguageLimitAlternativesToLocal";
static string const kTransliterationSetting = "TransliterationMode";
enum AlternativeMapLanguageHandling : uint8_t
{
  IgnoreAlternatives = 0,
  SystemOrder = 1,
  LocalOnly = 2,
};

struct Translation
{
  string m_translation = {};
  LanguageIndex m_likelyLanguageIndex = kUnsupportedLanguageIndex;
};

struct NameTranslation
{
  optional<string> m_primary = {};
  LanguageIndex m_primaryLikelyLanguageIndex = kUnsupportedLanguageIndex;
  optional<string> m_secondary = {};
  LanguageIndex m_secondaryLikelyLanguageIndex = kUnsupportedLanguageIndex;
};

// Order is important. Any reordering breaks backward compatibility.
// Languages with code |kReservedLanguageCode| may be used for another language after
// several data releases.
// Note that it's not feasible to increase languages number here due to current encoding (6 bit to
// store language code).
static array<Language, kMaxSupportedLanguages> const kLanguages = {
    {{"default", "Default Name", {"Any-Latin"}},
     {"en", "English", {}},
     {"ja", "日本語", {}},
     {"fr", "Français", {}},
     {"ko_rm", "Korean (Romanized)", {"Korean-Latin/BGN"}},
     {"ar", "العربية", {"Any-Latin"}},
     {"de", "Deutsch", {}},
     {"int_name", "International Name", {"Any-Latin"}},
     {"ru", "Русский", {"Russian-Latin/BGN"}},
     {"sv", "Svenska", {}},
     {"zh", "中文", {"Any-Latin"}},
     {"fi", "Suomi", {}},
     {"be", "Беларуская", {"Belarusian-Latin/BGN"}},
     {"ka", "ქართული", {"Georgian-Latin"}},
     {"ko", "한국어", {"Hangul-Latin/BGN"}},
     {"he", "עברית", {"Hebrew-Latin"}},
     {"nl", "Nederlands", {}},
     {"ga", "Gaeilge", {}},
     {"ja_rm", "Japanese (Romanized)", {"Any-Latin"}},
     {"el", "Ελληνικά", {"Greek-Latin"}},
     {"it", "Italiano", {}},
     {"es", "Español", {}},
     {"zh_pinyin", "Chinese (Pinyin)", {"Any-Latin"}},
     {"th", "ไทย", {}},  // Thai-Latin
     {"cy", "Cymraeg", {}},
     {"sr", "Српски", {"Serbian-Latin/BGN"}},
     {"uk", "Українська", {"Ukrainian-Latin/BGN"}},
     {"ca", "Català", {}},
     {"hu", "Magyar", {}},
     {kReservedLanguageCode /* Was "hsb" */, "", {}},
     {"eu", "Euskara", {}},
     {"fa", "فارسی", {"Any-Latin"}},
     {kReservedLanguageCode /* Was "br" */, "", {}},
     {"pl", "Polski", {}},
     {"hy", "Հայերէն", {"Armenian-Latin"}},
     {kReservedLanguageCode /* Was "kn" */, "", {}},
     {"sl", "Slovenščina", {}},
     {"ro", "Română", {}},
     {"sq", "Shqip", {}},
     {"am", "አማርኛ", {"Amharic-Latin/BGN"}},
     {"no", "Norsk", {}},
     {"cs", "Čeština", {}},
     {"id", "Bahasa Indonesia", {}},
     {"sk", "Slovenčina", {}},
     {"af", "Afrikaans", {}},
     {"ja_kana", "日本語(カタカナ)", {"Katakana-Latin", "Hiragana-Latin"}},
     {kReservedLanguageCode /* Was "lb" */, "", {}},
     {"pt", "Português", {}},
     {"hr", "Hrvatski", {}},
     {"da", "Dansk", {}},
     {"vi", "Tiếng Việt", {}},
     {"tr", "Türkçe", {}},
     {"bg", "Български", {"Bulgarian-Latin/BGN"}},
     {"alt_name", "Alternative Name", {"Any-Latin"}},
     {"lt", "Lietuvių", {}},
     {"old_name", "Old Name", {"Any-Latin"}},
     {"kk", "Қазақ", {"Kazakh-Latin/BGN"}},
     {"mr", "मराठी", {"Any-Latin"}},
     {"et", "Eesti", {}},
     {"ku", "Kurdish", {"Any-Latin"}},
     {"mn", "Mongolian", {"Mongolian-Latin/BGN"}},
     {"mk", "Македонски", {"Macedonian-Latin/BGN"}},
     {"lv", "Latviešu", {}},
     {"hi", "हिन्दी", {"Any-Latin"}}}};
static_assert(kLanguages.size() == kMaxSupportedLanguages,
              "With current encoding we are limited to 64 languages max. And we need kLanguages.size() to be exactly "
              "64 for backward compatibility.");

constexpr bool IsServiceLanguage(LanguageCode const languageCode)
{
  return languageCode == kLanguages[kDefaultNameIndex].m_languageCode ||
         languageCode == kLanguages[kInternationalNameIndex].m_languageCode ||
         languageCode == kLanguages[kAlternativeNameIndex].m_languageCode ||
         languageCode == kLanguages[kOldNameIndex].m_languageCode;
}

constexpr bool IsAlternativeOrOldName(LanguageIndex const languageIndex)
{
  return languageIndex == kAlternativeNameIndex || languageIndex == kOldNameIndex;
}

constexpr vector<Language> GetSupportedLanguages(bool const isIncludingServiceLanguages = true)
{
  vector<Language> languages;
  for (Language const & language : kLanguages)
    if (language.m_languageCode != kReservedLanguageCode &&
        (isIncludingServiceLanguages || !IsServiceLanguage(language.m_languageCode)))
      languages.push_back(language);
  return languages;
}

constexpr bool IsSupportedLanguageCode(LanguageCode const languageCode)
{
  if (languageCode == kReservedLanguageCode)
    return false;

  for (size_t i = 0; i < kMaxSupportedLanguages; ++i)
  {
    LanguageCode const existingLanguageCode = kLanguages[i].m_languageCode;
    if (languageCode == existingLanguageCode && kReservedLanguageCode != existingLanguageCode)
      return true;
  }
  return false;
}

constexpr bool IsSupportedLanguageIndex(LanguageIndex const languageIndex)
{
  return languageIndex >= 0 && languageIndex < kMaxSupportedLanguages &&
         kLanguages[languageIndex].m_languageCode != kReservedLanguageCode;
}

constexpr LanguageCode ConvertLanguageIndexToLanguageCode(LanguageIndex const languageIndex)
{
  if (!IsSupportedLanguageIndex(languageIndex))
    return {};

  return kLanguages[languageIndex].m_languageCode;
}

constexpr LanguageIndex ConvertLanguageCodeToLanguageIndex(LanguageCode const languageCode)
{
  if (languageCode == kReservedLanguageCode)
    return kUnsupportedLanguageIndex;

  for (LanguageIndex languageIndex = 0; languageIndex < kMaxSupportedLanguages; ++languageIndex)
    if (languageCode == kLanguages[languageIndex].m_languageCode)
      return languageIndex;

  return kUnsupportedLanguageIndex;
}

constexpr vector<LanguageCode> ConvertLanguageIndexesToLanguageCodes(vector<LanguageIndex> const languageIndexes)
{
  vector<LanguageCode> languageCodes = {};
  for (LanguageIndex const & languageIndex : languageIndexes)
    languageCodes.push_back(ConvertLanguageIndexToLanguageCode(languageIndex));
  return languageCodes;
}

constexpr vector<LanguageIndex> ConvertLanguageCodesToLanguageIndexes(vector<LanguageCode> const languageCodes)
{
  vector<LanguageIndex> languageIndexes = {};
  for (LanguageCode const & languageCode : languageCodes)
    languageIndexes.push_back(ConvertLanguageCodeToLanguageIndex(languageCode));
  return languageIndexes;
}

optional<LanguageCode> GetCustomMapLanguageCode();
AlternativeMapLanguageHandling UsedAlternativeMapLanguageHandling();
bool ShouldUseTransliteration();

vector<LanguageCode> GetSystemLanguageCodes();
LanguageCode GetInterfaceLanguageCode();
vector<LanguageCode> GetSimplifiedSystemLanguageCodes();
vector<LanguageIndex> GetSimplifiedSystemLanguageIndexes();
vector<LanguageCode> GetMapLanguageCodes();
LanguageCode GetMapLanguageCode();
vector<LanguageIndex> GetMapLanguageIndexes();
LanguageIndex GetMapLanguageIndex();

constexpr string GetLanguageNameByLanguageIndex(LanguageIndex const languageIndex)
{
  if (!IsSupportedLanguageIndex(languageIndex))
    return {};

  return kLanguages[languageIndex].m_name;
}

constexpr string GetLanguageNameByLanguageCode(LanguageCode const languageCode)
{
  return GetLanguageNameByLanguageIndex(ConvertLanguageCodeToLanguageIndex(languageCode));
}

constexpr vector<TransliteratorId> GetTransliteratorsIdsByLanguageIndex(LanguageIndex const languageIndex)
{
  if (!IsSupportedLanguageIndex(languageIndex))
    return {};

  return kLanguages[languageIndex].m_transliteratorsIds;
}

constexpr vector<TransliteratorId> GetTransliteratorsIdsByLanguageCode(LanguageCode const languageCode)
{
  return GetTransliteratorsIdsByLanguageIndex(ConvertLanguageCodeToLanguageIndex(languageCode));
}

constexpr vector<LanguageIndex> SimilarLanguageIndexes(LanguageIndex const languageIndex)
{
  if (languageIndex == kJapaneseLanguageIndex)
    return {kJapaneseKatakanaLanguageIndex, kJapaneseRomanizedLanguageIndex};
  else if (languageIndex == kKoreanLanguageIndex)
    return {kKoreanRomanizedLanguageIndex};
  else if (languageIndex == kChineseLanguageIndex)
    return {kChinesePinyinLanguageIndex};
  else if (languageIndex == kBelarusianLanguageIndex)
    return {kRussianLanguageIndex};
  else
    return {};
}

constexpr LanguageIndex MatchingRegionalLanguageIndex(vector<LanguageIndex> const regionalLanguageIndexes,
                                                      vector<LanguageIndex> const languageIndexes)
{
  for (LanguageIndex const languageIndex : languageIndexes)
  {
    if (find(regionalLanguageIndexes.begin(), regionalLanguageIndexes.end(), languageIndex) !=
        regionalLanguageIndexes.end())
      return languageIndex;

    for (LanguageIndex const similarLanguageIndex : SimilarLanguageIndexes(languageIndex))
      if (find(regionalLanguageIndexes.begin(), regionalLanguageIndexes.end(), similarLanguageIndex) !=
          regionalLanguageIndexes.end())
        return similarLanguageIndex;
  }

  return kDefaultNameIndex;
}

vector<LanguageIndex> PrioritizedMapLanguageIndexes(vector<LanguageIndex> const regionalLanguageIndexes = {});

}  // namespace localisation
