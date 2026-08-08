#include "indexer/road_shields_parser.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

#include "3party/ankerl/unordered_dense.h"

/*
 * TODO : why all this parsing happens in the run-time? The most of it should be moved to the generator.
 * E.g. now the ref contains
 *    ee:national/8;e-road/E 67;ee:local/7841171
 * where the latter part is not being used at all.
 * The generator should produce just
 *    x8;yE 67
 * where x/y are one byte road shield type codes.
 */

namespace ftypes
{
namespace
{

// Used to discard possible bogus values e.g. in USRoadShieldParser
uint32_t constexpr kMaxRoadShieldBytesSize = 10;

std::array<std::string, 2> const kFederalCode = {{"US", "FSR"}};

std::array<std::string, 61> const kStatesCode = {{
    "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "DC", "FL", "GA", "HI", "ID", "IL", "IN",
    "IA", "KS", "KY", "LA", "ME", "MD", "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH",
    "NJ", "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC", "SD", "TN", "TX", "UT",
    "VT", "VA", "WA", "WV", "WI", "WY", "AS", "GU", "MP", "PR", "VI", "UM", "FM", "MH", "PW",

    "SR",  // common prefix for State Road
}};

std::array<std::string, 17> const kModifiers = {{"alt", "alternate", "bus", "business", "bypass", "historic",
                                                 "connector", "loop", "scenic", "spur", "temporary", "toll", "truck",
                                                 "north", "south", "east", "west"}};

std::array<std::string, 27> const kBrazilStatesCode = {{
    "AC", "AL", "AP", "AM", "BA", "CE", "DF", "ES", "GO", "MA", "MT", "MS", "MG", "PA",
    "PB", "PR", "PE", "PI", "RJ", "RN", "RS", "RO", "RR", "SC", "SP", "SE", "TO",
}};

// Shields based on a network tag in a route=road relation.
ankerl::unordered_dense::map<std::string, RoadShieldType> const kRoadNetworkShields = {
    // International road networks.
    {"e-road", RoadShieldType::Generic_Green},  // E 105
    {"asianhighway", RoadShieldType::Hidden},   // AH8. Blue, but usually not posted.
    // National and regional networks for some countries.
    {"ru:national", RoadShieldType::Generic_Blue},
    {"ru:regional", RoadShieldType::Generic_Blue},
    {"bg:national", RoadShieldType::Generic_Green},
    {"bg:regional", RoadShieldType::Generic_Blue},
    {"by:national", RoadShieldType::Generic_Red},
    // https://github.com/organicmaps/organicmaps/issues/3083
    //{"by:regional", RoadShieldType::Generic_Red},
    {"co:national", RoadShieldType::Generic_White_Bordered},
    {"cz:national", RoadShieldType::Generic_Red},
    {"cz:regional", RoadShieldType::Generic_Blue},
    // Estonia parser produces more specific shield types, incl. Generic_Orange.
    //{"ee:national", RoadShieldType::Generic_Red},
    //{"ee:regional", RoadShieldType::Generic_White_Bordered},
    {"in:ne", RoadShieldType::Generic_Blue},
    {"in:nh", RoadShieldType::Generic_Orange_Bordered},
    {"in:sh", RoadShieldType::Generic_Green},
    {"fr:a-road", RoadShieldType::Generic_Red},
    {"jp:national", RoadShieldType::Generic_Blue},
    {"jp:regional", RoadShieldType::Generic_Blue},
    {"jp:prefectural", RoadShieldType::Generic_Blue},
    {"lt:national", RoadShieldType::Generic_Red},
    {"lt:regional", RoadShieldType::Generic_Blue},
    {"lv:national", RoadShieldType::Generic_Red},
    {"lv:regional", RoadShieldType::Generic_Blue},
    {"pl:national", RoadShieldType::Generic_Red},
    {"pl:regional", RoadShieldType::Generic_Orange_Bordered},
    {"pl:local", RoadShieldType::Generic_White_Bordered},
    {"ua:national", RoadShieldType::Generic_Blue},
    {"ua:regional", RoadShieldType::Generic_Blue},
    {"ua:territorial", RoadShieldType::Generic_White_Bordered},
    {"ua:local", RoadShieldType::Generic_White_Bordered},
    {"uy", RoadShieldType::UY_National},
    {"za:national", RoadShieldType::Generic_White_Bordered},
    {"za:regional", RoadShieldType::Generic_White_Bordered},
    {"my:federal", RoadShieldType::Generic_Orange_Bordered},
    {"ar:national", RoadShieldType::Argentina_RN},
    {"bo:fundamental", RoadShieldType::Bolivia_Fundamental},
    // United States road networks.
    {"us:i", RoadShieldType::US_Interstate},
    {"us:us", RoadShieldType::US_Highway},
    {"us:sr", RoadShieldType::US_Highway},
    {"us:fsr", RoadShieldType::US_Highway},
};

class RoadShieldParser
{
public:
  explicit RoadShieldParser(std::string const & baseRoadNumber) : m_baseRoadNumber(baseRoadNumber) {}
  virtual ~RoadShieldParser() = default;
  virtual RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const = 0;

  RoadShieldType FindNetworkShield(std::string network) const
  {
    // Special processing for US state highways, to not duplicate the table.
    if (network.size() == 5 && network.starts_with("US:"))
    {
      if (base::IsExist(kStatesCode, network.substr(3)))
        return RoadShieldType::Generic_White_Bordered;
    }

    // Minimum length for network tag is 2 (UY).
    if (network.size() >= 2)
    {
      strings::AsciiToLower(network);

      // Cut off suffixes after a semicolon repeatedly, until we find a relevant shield.
      auto semicolonPos = network.size();
      while (semicolonPos != std::string::npos)
      {
        network.resize(semicolonPos);  // cut off the ":xxx" suffix
        auto const it = kRoadNetworkShields.find(network);
        if (it != kRoadNetworkShields.cend())
          return it->second;
        semicolonPos = network.rfind(':');
      }
    }
    return RoadShieldType::Default;
  }

  RoadShieldsSetT GetRoadShields() const
  {
    RoadShieldsSetT result, defaultShields;

    uint8_t index = 0;
    strings::Tokenize(m_baseRoadNumber, ";", [&](std::string_view rawText)
    {
      ++index;
      RoadShield shield;
      auto slashPos = rawText.find('/');
      if (slashPos == std::string::npos)
      {
        shield = ParseRoadShield(rawText, index);
      }
      else
      {
        shield = ParseRoadShield(rawText.substr(slashPos + 1), index);
        // TODO: use a network-based shield type override only if a parser couldn't make it
        // more specific than country's default shield type.
        // E.g. "94" is set to Generic_Orange by Estonia parser, but then
        // is overriden by "ee:national" => Generic_Red.
        // (can't override just RoadShieldType::Default, as e.g. Russia parser uses Generic_Blue as country default).
        if (shield.m_type != RoadShieldType::Hidden)
        {
          RoadShieldType const networkType = FindNetworkShield(std::string(rawText.substr(0, slashPos)));
          if (networkType != RoadShieldType::Default)
            shield.m_type = networkType;
        }
      }
      if (!shield.m_name.empty() && shield.m_type != RoadShieldType::Hidden)
      {
        if (shield.m_type != RoadShieldType::Default)
        {
          // Schedule deletion of a shield with the same text and default style, if present.
          defaultShields.emplace_back(RoadShieldType::Default, shield.m_name, shield.m_additionalText);
        }
        result.push_back(std::move(shield));
      }
    });

    result.erase_if([&defaultShields](RoadShield const & shield)
    { return std::find(defaultShields.begin(), defaultShields.end(), shield) != defaultShields.end(); });

    // Remove duplicates with same type and text
    for (size_t i = 0; i < result.size(); ++i)
    {
      for (size_t j = i + 1; j < result.size(); ++j)
      {
        if (result[i].m_type == result[j].m_type && result[i].m_name == result[j].m_name)
        {
          result.erase(result.begin() + j);
          --j;
        }
      }
    }

    return result;
  }

protected:
  std::string const m_baseRoadNumber;
};

class USRoadShieldParser : public RoadShieldParser
{
public:
  explicit USRoadShieldParser(std::string const & baseRoadNumber) : RoadShieldParser(baseRoadNumber) {}
  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    std::string shieldText(rawText);

    std::replace(shieldText.begin(), shieldText.end(), '-', ' ');
    auto const shieldParts = strings::Tokenize(shieldText, " ");

    // Process long road shield titles to skip invalid data.
    if (shieldText.size() > kMaxRoadShieldBytesSize)
    {
      std::string lowerShieldText = shieldText;
      strings::AsciiToLower(lowerShieldText);

      bool modifierFound = false;
      for (auto const & modifier : kModifiers)
      {
        if (lowerShieldText.find(modifier) != std::string::npos)
        {
          modifierFound = true;
          break;
        }
      }
      if (!modifierFound)
        return RoadShield();
    }

    if (shieldParts.size() <= 1)
      return RoadShield(RoadShieldType::Default, rawText);

    std::string_view const roadType = shieldParts[0];  // 'I' for interstates and kFederalCode/kStatesCode for highways.
    std::string roadNumber(shieldParts[1]);
    std::string additionalInfo;
    if (shieldParts.size() >= 3)
    {
      additionalInfo = shieldParts[2];
      // Process cases like "US Loop 16".
      if (!strings::IsASCIINumeric(shieldParts[1]) && strings::IsASCIINumeric(shieldParts[2]))
      {
        roadNumber = shieldParts[2];
        additionalInfo = shieldParts[1];
      }
    }

    if (roadType == "I")
      return RoadShield(RoadShieldType::US_Interstate, roadNumber, additionalInfo);

    if (base::IsExist(kFederalCode, shieldParts[0]))
      return RoadShield(RoadShieldType::US_Highway, roadNumber, additionalInfo);

    if (base::IsExist(kStatesCode, shieldParts[0]))
      return RoadShield(RoadShieldType::Generic_White_Bordered, roadNumber, additionalInfo);

    return RoadShield(RoadShieldType::Default, rawText);
  }
};

class IndiaRoadShieldParser : public RoadShieldParser
{
public:
  explicit IndiaRoadShieldParser(std::string const & baseRoadNumber) : RoadShieldParser(baseRoadNumber) {}
  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    std::string shieldText(rawText);

    std::erase_if(shieldText, [](char c) { return c == '-' || strings::IsASCIISpace(c); });

    if (shieldText.size() <= 2)
      return RoadShield(RoadShieldType::Default, rawText);

    std::string_view roadType = std::string_view(shieldText).substr(0, 2);
    std::string_view roadNumber = std::string_view(shieldText).substr(2);

    if (roadType == "NE")
      return RoadShield(RoadShieldType::Generic_Blue, roadNumber);

    if (roadType == "NH")
      return RoadShield(RoadShieldType::Generic_Orange, roadNumber);

    if (roadType == "SH")
      return RoadShield(RoadShieldType::Generic_Green, roadNumber);

    return RoadShield(RoadShieldType::Default, rawText);
  }
};

class DefaultTypeRoadShieldParser : public RoadShieldParser
{
public:
  DefaultTypeRoadShieldParser(std::string const & baseRoadNumber, RoadShieldType const & defaultType)
    : RoadShieldParser(baseRoadNumber)
    , m_type(defaultType)
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    return RoadShield(m_type, rawText);
  }

private:
  RoadShieldType const m_type;
};

// Matches by a list of given substrings.
// If several substrings are present, then the leftmost wins.
class SimpleRoadShieldParser : public RoadShieldParser
{
public:
  struct Entry
  {
    Entry() = default;
    Entry(std::string_view name, RoadShieldType type, bool isRedundant = false, bool shouldTrimName = false)
      : m_name(name)
      , m_type(type)
      , m_isRedundant(isRedundant)
      , m_shouldTrimName(shouldTrimName)
    {}

    std::string_view m_name;
    RoadShieldType m_type = RoadShieldType::Default;
    /* Hides a specific secondary etc. sign, if there is a primary one */
    bool m_isRedundant = false;
    bool m_shouldTrimName = false;
  };

  using ShieldTypes = buffer_vector<Entry, 8>;

  SimpleRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes && types,
                         RoadShieldType defaultType = RoadShieldType::Default)
    : RoadShieldParser(baseRoadNumber)
    , m_types(std::move(types))
    , m_defaultType(defaultType)
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    RoadShieldType type = m_defaultType;
    std::string name = std::string{rawText};
    size_t idx = std::numeric_limits<size_t>::max();
    for (auto const & p : m_types)
    {
      auto const i = rawText.find(p.m_name);
      if (i != std::string::npos && i < idx)
      {
        name = std::string{rawText};
        if (p.m_shouldTrimName)
        {
          strings::ReplaceFirst(name, std::string{p.m_name}, "");
          strings::Trim(name);
        }
        if (index != 1 && p.m_isRedundant)
          type = RoadShieldType::Hidden;
        else
          type = p.m_type;
        idx = i;
      }
    }
    return {type, name};
  }

private:
  ShieldTypes const m_types;
  RoadShieldType const m_defaultType;
};

// Matches by a list of given highway classes for the first shield.
// Falls back to matching by a list of given substrings (identical to SimpleRoadShieldParser) for all other shields.
class HighwayClassRoadShieldParser : public RoadShieldParser
{
public:
  struct Entry
  {
    Entry() = default;
    Entry(std::string_view name, HighwayClass highwayClass, RoadShieldType type, bool isRedundant = false,
          bool shouldTrimName = false)
      : m_name(name)
      , m_type(type)
      , m_highwayClass(highwayClass)
      , m_isRedundant(isRedundant)
      , m_shouldTrimName(shouldTrimName)
    {}

    std::string_view m_name;
    RoadShieldType m_type = RoadShieldType::Default;
    HighwayClass m_highwayClass = HighwayClass::Undefined;
    /* Hides a specific secondary etc. sign, if there is a primary one */
    bool m_isRedundant = false;
    bool m_shouldTrimName = false;
  };

  using ShieldTypes = buffer_vector<Entry, 8>;

  HighwayClassRoadShieldParser(std::string const & baseRoadNumber, HighwayClass highwayClass, ShieldTypes && types,
                               RoadShieldType defaultType = RoadShieldType::Default)
    : RoadShieldParser(baseRoadNumber)
    , m_highwayClass(highwayClass)
    , m_types(std::move(types))
    , m_defaultType(defaultType)
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    if (index == 1)
    {
      for (auto const & p : m_types)
      {
        if (p.m_highwayClass == m_highwayClass)
        {
          std::string name = std::string{rawText};
          if (p.m_shouldTrimName)
          {
            strings::ReplaceFirst(name, std::string{p.m_name}, "");
            strings::Trim(name);
          }
          return RoadShield(p.m_type, name);
        }
      }
    }

    SimpleRoadShieldParser::ShieldTypes simpleShieldTypes = {};
    for (auto const & p : m_types)
    {
      simpleShieldTypes.push_back(
          SimpleRoadShieldParser::Entry(p.m_name, p.m_type, p.m_isRedundant, p.m_shouldTrimName));
    }
    return SimpleRoadShieldParser(m_baseRoadNumber, std::move(simpleShieldTypes), m_defaultType)
        .ParseRoadShield(rawText, index);
  }

private:
  HighwayClass const m_highwayClass;
  ShieldTypes const m_types;
  RoadShieldType const m_defaultType;
};

uint16_t constexpr kAnyHigherRoadNumber = std::numeric_limits<uint16_t>::max();

// Matches by a list of numeric ranges (a first matching range is used).
// Non-numbers and numbers out of any range are matched to RoadShieldType::Default.
// Use kAnyHigherRoadNumber to match any number higher than a specified lower bound.
class NumericRoadShieldParser : public RoadShieldParser
{
public:
  struct Entry
  {
    Entry() = default;
    Entry(uint16_t low, uint16_t high, RoadShieldType type) : m_low(low), m_high(high), m_type(type) {}

    uint16_t m_low, m_high;
    RoadShieldType m_type = RoadShieldType::Default;
  };

  // A map of {lower_bound, higher_bound} -> RoadShieldType.
  using ShieldTypes = buffer_vector<Entry, 8>;

  NumericRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes && types)
    : RoadShieldParser(baseRoadNumber)
    , m_types(std::move(types))
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    uint64_t ref;
    if (strings::to_uint(rawText, ref))
    {
      for (auto const & p : m_types)
        if (p.m_low <= ref && (ref <= p.m_high || p.m_high == kAnyHigherRoadNumber))
          return RoadShield(p.m_type, rawText);
    }

    return RoadShield(RoadShieldType::Default, rawText);
  }

private:
  ShieldTypes const m_types;
};

class SimpleUnicodeRoadShieldParser : public RoadShieldParser
{
public:
  struct Entry
  {
    Entry() = default;
    Entry(std::string_view simpleName, std::string_view unicodeName, RoadShieldType type)
      : m_simpleName(simpleName)
      , m_unicodeName(unicodeName)
      , m_type(type)
    {
      ASSERT_NOT_EQUAL(simpleName, unicodeName, ());
      ASSERT_LESS_OR_EQUAL(simpleName.size(), unicodeName.size(), ());
    }

    std::string_view m_simpleName;
    std::string_view m_unicodeName;
    RoadShieldType m_type = RoadShieldType::Default;
  };

  using ShieldTypes = buffer_vector<Entry, 8>;

  SimpleUnicodeRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes && types,
                                RoadShieldType defaultType = RoadShieldType::Default)
    : RoadShieldParser(baseRoadNumber)
    , m_types(std::move(types))
    , m_defaultType(defaultType)
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    uint32_t constexpr kMaxRoadShieldSymbolsSize = 4 * kMaxRoadShieldBytesSize;

    if (rawText.size() > kMaxRoadShieldSymbolsSize)
      return RoadShield();

    for (auto const & p : m_types)
    {
      if (rawText.find(p.m_simpleName) != std::string::npos)
        return RoadShield(p.m_type, rawText);

      if (rawText.find(p.m_unicodeName) != std::string::npos)
        return RoadShield(p.m_type, rawText);
    }

    return RoadShield(m_defaultType, rawText);
  }

private:
  ShieldTypes const m_types;
  RoadShieldType const m_defaultType;
};

// Implementations of "ref" parses for some countries.

class AustriaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit AustriaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Generic_Blue_Bordered},
                                              {"S", RoadShieldType::Generic_Blue_Bordered},
                                              {"B", RoadShieldType::Generic_Blue, false, true},
                                              {"P", RoadShieldType::Generic_Pill_Red_Bordered},
                                              {"L", RoadShieldType::Generic_Pill_White_Bordered, false, true}})
  {}
};

class BelgiumRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit BelgiumRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber,
                             {{"A", RoadShieldType::Generic_White_Bordered}, {"N", RoadShieldType::Generic_Blue}})
  {}
};

class GreeceRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit GreeceRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber,
                             {{"Α", RoadShieldType::Highway_Hexagon_Green}, {"Ε", RoadShieldType::Generic_Blue}})
  {}
};

class IrelandRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit IrelandRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"M", RoadShieldType::Generic_Blue},
                                              {"N", RoadShieldType::UK_Highway},
                                              {"R", RoadShieldType::Generic_White_Bordered},
                                              {"L", RoadShieldType::Generic_White_Bordered}})
  {}
};

class ItalyRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit ItalyRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Italy_Autostrada},
                                              {"T", RoadShieldType::Italy_Autostrada},
                                              {"RA", RoadShieldType::Generic_Green_Bordered},
                                              {"NSA", RoadShieldType::Generic_Blue_Bordered},
                                              {"SS", RoadShieldType::Generic_Blue_Bordered},
                                              {"SR", RoadShieldType::Generic_Blue_Bordered},
                                              {"SP", RoadShieldType::Generic_Blue_Bordered}})
  {}
};

class TurkeyRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit TurkeyRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber,
                             {{"O", RoadShieldType::Highway_Hexagon_Turkey}, {"D", RoadShieldType::Generic_Blue}})
  {}
};

class HungaryRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit HungaryRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"M", RoadShieldType::Hungary_Blue}}, RoadShieldType::Hungary_Green)
  {}
};

class MoldovaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit MoldovaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"M", RoadShieldType::Generic_Red}, {"R", RoadShieldType::Generic_Blue}})
  {}
};

class PortugalRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit PortugalRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Generic_Blue},
                                              {"N", RoadShieldType::Generic_White_Bordered},
                                              {"EN", RoadShieldType::Generic_White_Bordered},
                                              {"R", RoadShieldType::Generic_Orange},
                                              {"IP", RoadShieldType::Generic_Red},
                                              {"IC", RoadShieldType::Generic_White_Bordered},
                                              {"EM", RoadShieldType::Generic_Orange},
                                              {"CM", RoadShieldType::Generic_Orange}})
  {}
};

class RomaniaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit RomaniaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Generic_Green},
                                              {"DN", RoadShieldType::Generic_Red},
                                              {"DJ", RoadShieldType::Generic_Blue},
                                              {"DC", RoadShieldType::Generic_Blue}})
  {}
};

class SerbiaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit SerbiaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Highway_Hexagon_Green}})
  {}
};

class SlovakiaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit SlovakiaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"D", RoadShieldType::Generic_Red}, {"R", RoadShieldType::Generic_Red}})
  {}
};

class SloveniaRoadShieldParser : public RoadShieldParser
{
public:
  SloveniaRoadShieldParser(std::string const & baseRoadNumber, HighwayClass highwayClass)
    : RoadShieldParser(baseRoadNumber)
    , m_highwayClass(highwayClass)
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    // "avtoceste" (motorways)
    if (m_highwayClass == HighwayClass::Motorway && rawText.starts_with("A"))
      return RoadShield(RoadShieldType::Highway_Hexagon_Green, rawText);

    // "hitre ceste" (trunk roads)
    if (m_highwayClass == HighwayClass::Trunk && rawText.starts_with("H"))
      return RoadShield(RoadShieldType::Generic_Blue_Bordered, rawText);

    // show junction sections of motorway and trunk roads as pill-shaped
    if (m_highwayClass == HighwayClass::Motorway)
      return RoadShield(RoadShieldType::Generic_Pill_Green_Bordered, rawText);
    if (m_highwayClass == HighwayClass::Trunk)
      return RoadShield(RoadShieldType::Generic_Pill_Blue_Bordered, rawText);

    // "glavne ceste" (main roads: 1-11, 101-114) and regionalne ceste (regional roads: 201-941)
    if (m_highwayClass == HighwayClass::Primary || m_highwayClass == HighwayClass::Secondary ||
        m_highwayClass == HighwayClass::Tertiary)
      return RoadShield(RoadShieldType::Generic_Orange_Bordered, rawText);

    return RoadShield(RoadShieldType::Generic_White_Bordered, rawText);
  }

private:
  HighwayClass const m_highwayClass;
};

class SwitzerlandRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit SwitzerlandRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Highway_Hexagon_Red}})
  {}
};

class LiechtensteinRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit LiechtensteinRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Highway_Hexagon_Red}})
  {}
};

class RussiaRoadShieldParser : public DefaultTypeRoadShieldParser
{
public:
  explicit RussiaRoadShieldParser(std::string const & baseRoadNumber)
    : DefaultTypeRoadShieldParser(baseRoadNumber, RoadShieldType::Generic_Blue)
  {}
};

class SpainRoadShieldParser : public DefaultTypeRoadShieldParser
{
public:
  explicit SpainRoadShieldParser(std::string const & baseRoadNumber)
    : DefaultTypeRoadShieldParser(baseRoadNumber, RoadShieldType::Generic_Blue)
  {}
};

class UKRoadShieldParser : public HighwayClassRoadShieldParser
{
public:
  explicit UKRoadShieldParser(std::string const & baseRoadNumber, HighwayClass const & highwayClass)
    : HighwayClassRoadShieldParser(baseRoadNumber, highwayClass,
                                   {{"M", HighwayClass::Motorway, RoadShieldType::Generic_Blue, true},
                                    {"E", HighwayClass::Motorway, RoadShieldType::Hidden},
                                    {"A", HighwayClass::Trunk, RoadShieldType::UK_Highway, true},
                                    {"A", HighwayClass::Primary, RoadShieldType::Generic_White_Bordered},
                                    {"B", HighwayClass::Secondary, RoadShieldType::Generic_White_Bordered}})
  {}
};

class FranceRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit FranceRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Generic_Red},
                                              {"N", RoadShieldType::Generic_Red},
                                              {"E", RoadShieldType::Generic_Green},
                                              {"D", RoadShieldType::Generic_Orange},
                                              {"M", RoadShieldType::Generic_Blue}})
  {}
};

class GermanyRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit GermanyRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A ", RoadShieldType::Highway_Hexagon_Blue, false, true},
                                              {"D ", RoadShieldType::Hidden},
                                              {"B ", RoadShieldType::Generic_Orange_Bordered},
                                              {"L", RoadShieldType::Generic_White_Bordered},
                                              {"K", RoadShieldType::Generic_White_Bordered}})
  {}
};

class ArgentinaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  // Hide duplicated shields to remove duplication due to tagging convention in AR:
  // https://wiki.openstreetmap.org/wiki/ES:Argentina/V%C3%ADas_de_circulaci%C3%B3n#Relaciones_de_Ruta_(type=route)
  // refs that don't start with RN/RP will still appear with the default shield (but shouldn't exist in AR)
  // suggestion for future improvement by @pastk: https://codeberg.org/comaps/comaps/pulls/3966#issuecomment-12533514
  explicit ArgentinaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"RN", RoadShieldType::Hidden}, {"RP", RoadShieldType::Hidden}})
  {}
};

class BoliviaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  // Hide duplicated shields to remove duplication due to tagging convention in Bolivia for national roads
  // TODO: Same improvements as outlined for the ArgentinaRoadShieldParser apply
  explicit BoliviaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"F", RoadShieldType::Hidden}})
  {}
};

class BrazilRoadShieldParser : public RoadShieldParser
{
public:
  explicit BrazilRoadShieldParser(std::string const & baseRoadNumber) : RoadShieldParser(baseRoadNumber) {}

  // Refs look like "BR-116" (federal) or "RS-410"/"SP-280" (state), occasionally with a space
  // instead of a dash: https://wiki.openstreetmap.org/wiki/Brazil/Highway_classification
  // Only the number is displayed as text: the "BR" lettering is drawn into the federal symbol,
  // and each state has its own symbol with the state code also drawn
  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    std::string shieldText(rawText);
    std::replace(shieldText.begin(), shieldText.end(), '-', ' ');
    auto const parts = strings::Tokenize(shieldText, " ");

    // Keep leading zeros
    if (parts.size() != 2 || parts[1].size() > 3 || !strings::IsASCIINumeric(parts[1]))
      return RoadShield(RoadShieldType::Default, rawText);

    if (parts[0] == "BR")
      return RoadShield(RoadShieldType::Brazil_National, std::string{parts[1]}, /* additionalText */ "",
                        /* shieldText */ "BR-" + std::string{parts[1]});

    std::string_view code = parts[0];
    // Some states add a letter to the code for special road classes, e.g. ERS/VRS/RSC
    // in Rio Grande do Sul, MGC/LMG/AMG in Minas Gerais, PRC in Paraná.
    // The signs display the plain state code.
    if (code.size() == 3)
    {
      if (base::IsExist(kBrazilStatesCode, code.substr(0, 2)))
        code = code.substr(0, 2);
      else if (base::IsExist(kBrazilStatesCode, code.substr(1)))
        code = code.substr(1);
    }

    if (base::IsExist(kBrazilStatesCode, code))
    {
      // "Coincident" state roads share the number of the federal highway they overlap
      // (e.g. ref="BR-453;RSC-453"): show only the federal shield then.
      if (IsCoincidentWithFederal(parts[1]))
        return RoadShield(RoadShieldType::Hidden, parts[1]);
      return RoadShield(RoadShieldType::Brazil_State, std::string{parts[1]}, std::string{code},
                        /* shieldText */ std::string{code} + "-" + std::string{parts[1]});
    }

    return RoadShield(RoadShieldType::Default, rawText);
  }

private:
  bool IsCoincidentWithFederal(std::string_view number) const
  {
    bool found = false;
    strings::Tokenize(m_baseRoadNumber, ";", [&](std::string_view token)
    {
      auto const slashPos = token.find('/');
      if (slashPos != std::string_view::npos)
        token = token.substr(slashPos + 1);
      if ((token.starts_with("BR-") || token.starts_with("BR ")) && token.substr(3) == number)
        found = true;
    });
    return found;
  }
};

class UkraineRoadShieldParser : public SimpleUnicodeRoadShieldParser
{
public:
  // The second parameter in the constructor is a cyrillic symbol.
  explicit UkraineRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleUnicodeRoadShieldParser(baseRoadNumber, {{"M", "М", RoadShieldType::Generic_Blue},
                                                     {"H", "Н", RoadShieldType::Generic_Blue},
                                                     {"P", "Р", RoadShieldType::Generic_Blue},
                                                     {"E", "Е", RoadShieldType::Generic_Green}})
  {}
};

class BelarusRoadShieldParser : public SimpleUnicodeRoadShieldParser
{
public:
  // The second parameter in the constructor is a cyrillic symbol.
  explicit BelarusRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleUnicodeRoadShieldParser(baseRoadNumber, {{"M", "М", RoadShieldType::Generic_Red},
                                                     {"P", "Р", RoadShieldType::Generic_Red},
                                                     {"E", "Е", RoadShieldType::Generic_Green}})
  {}
};

class LatviaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit LatviaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Generic_Red},
                                              {"E", RoadShieldType::Generic_Green},
                                              {"P", RoadShieldType::Generic_Blue},
                                              {"V", RoadShieldType::Generic_Grey_Bordered}})
  {}
};

class NetherlandsRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit NetherlandsRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{"A", RoadShieldType::Generic_Red},
                                              {"E", RoadShieldType::Generic_Green},
                                              {"N", RoadShieldType::Generic_Orange_Bordered}})
  {}
};

class NorwayRoadShieldParser : public RoadShieldParser
{
public:
  // Norway does as of July 12 2026 not use route relations,
  // hence the E-Roads are not automatically rendered as green shields
  // In addition, the state owned roads have green shields
  // https://wiki.openstreetmap.org/wiki/Norway/Highways#Tagging_conventions
  NorwayRoadShieldParser(std::string const & baseRoadNumber, HighwayClass highwayClass)
    : RoadShieldParser(baseRoadNumber)
    , m_highwayClass(highwayClass)
  {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t /* index */) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return {};

    std::string name(rawText);
    strings::Trim(name);

    bool isRingRoad = false;
    if (name.starts_with("Ring"))
    {
      auto const numberPos = name.find_first_not_of(' ', 4);
      isRingRoad = numberPos != std::string::npos && numberPos > 4 &&
                   strings::IsASCIINumeric(std::string_view(name).substr(numberPos));
    }
    if (isRingRoad)
      return {RoadShieldType::Generic_Pill_White_Bordered, name};

    if (name.size() > 1 && name.front() == 'E')
    {
      auto const numberPos = name.find_first_not_of(' ', 1);
      if (numberPos != std::string::npos && strings::IsASCIINumeric(std::string_view(name).substr(numberPos)))
        return {RoadShieldType::Generic_Green, name};
    }

    uint64_t roadNumber;
    if (!strings::to_uint(name, roadNumber))
      return {RoadShieldType::Default, name};

    switch (m_highwayClass)
    {
    case HighwayClass::Motorway:
    case HighwayClass::Trunk: return {RoadShieldType::Generic_Green, name};
    case HighwayClass::Primary: return {RoadShieldType::Generic_White_Bordered, name};
    // Four-digit county road numbers are unsigned, but still tagged with ref instead of unsigned_ref
    case HighwayClass::Secondary:
    case HighwayClass::LivingStreet: return {RoadShieldType::Hidden, name};
    default: return {RoadShieldType::Default, name};
    }
  }

private:
  HighwayClass const m_highwayClass;
};

class FinlandRoadShieldParser : public NumericRoadShieldParser
{
public:
  explicit FinlandRoadShieldParser(std::string const & baseRoadNumber)
    : NumericRoadShieldParser(baseRoadNumber, {{1, 30, RoadShieldType::Generic_Red},
                                               {40, 99, RoadShieldType::Generic_Orange_Bordered},
                                               {100, 999, RoadShieldType::Generic_White_Bordered},
                                               {1000, 9999, RoadShieldType::Generic_Blue},
                                               {10000, kAnyHigherRoadNumber, RoadShieldType::Hidden}})
  {}
};

class EstoniaRoadShieldParser : public NumericRoadShieldParser
{
public:
  explicit EstoniaRoadShieldParser(std::string const & baseRoadNumber)
    : NumericRoadShieldParser(baseRoadNumber, {{1, 11, RoadShieldType::Generic_Red},
                                               {12, 91, RoadShieldType::Generic_Orange_Bordered},
                                               {92, 92, RoadShieldType::Generic_Red},
                                               {93, 95, RoadShieldType::Generic_Orange_Bordered},
                                               {96, 999, RoadShieldType::Generic_White_Bordered},
                                               {1000, kAnyHigherRoadNumber, RoadShieldType::Hidden}})
  {}
};

class MalaysiaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit MalaysiaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber,
                             {{"AH", RoadShieldType::Generic_Blue}, {"E", RoadShieldType::Generic_Blue}},
                             RoadShieldType::Generic_Orange_Bordered)
  {}
};

class CyprusRoadShieldParser : public SimpleRoadShieldParser
{
public:
  explicit CyprusRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {                                                  // North Cyprus.
                                              {"D.", RoadShieldType::Generic_Blue},             // White font.
                                              {"GM.", RoadShieldType::Generic_White_Bordered},  // Blue font.
                                              {"GZ.", RoadShieldType::Generic_White_Bordered},  // Blue font.
                                              {"GR.", RoadShieldType::Generic_White_Bordered},  // Blue font.
                                              {"LF.", RoadShieldType::Generic_White_Bordered},  // Blue font.
                                              {"İK.", RoadShieldType::Generic_White_Bordered},  // Blue font.
                                                                                                // South Cyprus.
                                              {"A", RoadShieldType::Generic_Green},             // Yellow font. Hexagon.
                                              {"B", RoadShieldType::Generic_Blue},              // Yellow font.
                                              {"E", RoadShieldType::Generic_Blue},              // Yellow font.
                                              {"F", RoadShieldType::Generic_Blue},              // Yellow font.
                                              {"U", RoadShieldType::Generic_Blue}})             // Yellow font.
  {}
};

class MexicoRoadShieldParser : public RoadShieldParser
{
public:
  explicit MexicoRoadShieldParser(std::string const & baseRoadNumber) : RoadShieldParser(baseRoadNumber) {}

  RoadShield ParseRoadShield(std::string_view rawText, uint8_t index) const override
  {
    std::string shieldText(rawText);

    std::replace(shieldText.begin(), shieldText.end(), '-', ' ');
    auto const shieldParts = strings::Tokenize(shieldText, " ");

    if (shieldText.size() > kMaxRoadShieldBytesSize)
      return {};

    if (shieldParts.size() <= 1)
      return RoadShield(RoadShieldType::Default, rawText);

    std::string roadNumber(shieldParts[1]);
    std::string additionalInfo;
    if (shieldParts.size() >= 3)
    {
      additionalInfo = shieldParts[2];
      if (!strings::IsASCIINumeric(shieldParts[1]) && strings::IsASCIINumeric(shieldParts[2]))
      {
        roadNumber = shieldParts[2];
        additionalInfo = shieldParts[1];
      }
    }

    // Remove possible leading zero.
    if (strings::IsASCIINumeric(roadNumber) && roadNumber[0] == '0')
      roadNumber.erase(0);

    if (shieldParts[0] == "MEX")
      return RoadShield(RoadShieldType::Default, roadNumber, additionalInfo);

    return RoadShield(RoadShieldType::Default, rawText);
  }
};
}  // namespace

RoadShieldsSetT GetRoadShields(FeatureType & f)
{
  auto const & ref = f.GetRef();
  if (ref.empty())
    return {};

  auto const & highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(f));
  if (highwayClass == HighwayClass::Undefined)
    return {};

  std::string mwmName = f.GetID().GetMwmName();
  ASSERT(!mwmName.empty(), (f.GetID()));

  return GetRoadShields(mwmName, ref, highwayClass);
}

RoadShieldsSetT GetRoadShields(std::string_view mwmName, std::string const & roadNumber,
                               HighwayClass const & highwayClass)
{
  // Find out the country name.
  auto const underlinePos = mwmName.find('_');
  if (underlinePos != std::string::npos)
    mwmName = mwmName.substr(0, underlinePos);

  if (mwmName == "US")
    return USRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "UK")
    return UKRoadShieldParser(roadNumber, highwayClass).GetRoadShields();
  if (mwmName == "India")
    return IndiaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Austria")
    return AustriaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Argentina")
    return ArgentinaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Bolivia")
    return BoliviaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Brazil")
    return BrazilRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Belgium")
    return BelgiumRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Greece")
    return GreeceRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Ireland")
    return IrelandRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Italy")
    return ItalyRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Turkey")
    return TurkeyRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Hungary")
    return HungaryRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Moldova")
    return MoldovaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Portugal")
    return PortugalRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Romania")
    return RomaniaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Serbia")
    return SerbiaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Slovakia")
    return SlovakiaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Slovenia")
    return SloveniaRoadShieldParser(roadNumber, highwayClass).GetRoadShields();
  if (mwmName == "Switzerland")
    return SwitzerlandRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Liechtenstein")
    return LiechtensteinRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Russia")
    return RussiaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "France")
    return FranceRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Germany")
    return GermanyRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Spain")
    return SpainRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Ukraine")
    return UkraineRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Belarus")
    return BelarusRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Latvia")
    return LatviaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Netherlands")
    return NetherlandsRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Norway")
    return NorwayRoadShieldParser(roadNumber, highwayClass).GetRoadShields();
  if (mwmName == "Finland")
    return FinlandRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Estonia")
    return EstoniaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Malaysia")
    return MalaysiaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Mexico")
    return MexicoRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Cyprus")
    return CyprusRoadShieldParser(roadNumber).GetRoadShields();

  return SimpleRoadShieldParser(roadNumber, SimpleRoadShieldParser::ShieldTypes()).GetRoadShields();
}

RoadShieldsSetT GetRoadShields(std::string const & rawRoadNumber)
{
  if (rawRoadNumber.empty())
    return {};

  return SimpleRoadShieldParser(rawRoadNumber, SimpleRoadShieldParser::ShieldTypes()).GetRoadShields();
}

std::string GetRoadShieldDisplayRef(std::string const & rawRoadNumber)
{
  std::vector<std::string> displayRefs;
  strings::Tokenize(rawRoadNumber, ";", [&](std::string_view token)
  {
    if (auto const slash = token.find('/'); slash != std::string_view::npos)
      token.remove_prefix(slash + 1);
    if (!token.empty())
      displayRefs.emplace_back(token);
  });
  return strings::JoinStrings(displayRefs, ";");
}

std::vector<std::string> GetRoadShieldsNames(FeatureType & ft)
{
  std::vector<std::string> names;
  auto const & ref = ft.GetRef();
  if (!ref.empty() && IsStreetOrSquareChecker::Instance()(ft))
    for (auto && shield : GetRoadShields(ref))
      names.push_back(std::move(shield.m_name));
  return names;
}

std::string DebugPrint(RoadShieldType shieldType)
{
  using ftypes::RoadShieldType;
  switch (shieldType)
  {
  case RoadShieldType::Default: return "default";
  case RoadShieldType::Generic_White: return "white";
  case RoadShieldType::Generic_Green: return "green";
  case RoadShieldType::Generic_Blue: return "blue";
  case RoadShieldType::Generic_Red: return "red";
  case RoadShieldType::Generic_Orange: return "orange";
  case RoadShieldType::Generic_Grey: return "grey";
  case RoadShieldType::Generic_White_Bordered: return "white bordered";
  case RoadShieldType::Generic_Green_Bordered: return "green bordered";
  case RoadShieldType::Generic_Blue_Bordered: return "blue bordered";
  case RoadShieldType::Generic_Red_Bordered: return "red bordered";
  case RoadShieldType::Generic_Orange_Bordered: return "orange bordered";
  case RoadShieldType::Generic_Grey_Bordered: return "grey bordered";
  case RoadShieldType::Generic_Pill_White: return "white pill";
  case RoadShieldType::Generic_Pill_Green: return "green pill";
  case RoadShieldType::Generic_Pill_Blue: return "blue pill";
  case RoadShieldType::Generic_Pill_Red: return "red pill";
  case RoadShieldType::Generic_Pill_Orange: return "orange pill";
  case RoadShieldType::Generic_Pill_White_Bordered: return "white pill bordered";
  case RoadShieldType::Generic_Pill_Green_Bordered: return "green pill bordered";
  case RoadShieldType::Generic_Pill_Blue_Bordered: return "blue pill bordered";
  case RoadShieldType::Generic_Pill_Red_Bordered: return "red pill bordered";
  case RoadShieldType::Generic_Pill_Orange_Bordered: return "orange pill bordered";
  case RoadShieldType::Highway_Hexagon_Green: return "highway hexagon green";
  case RoadShieldType::Highway_Hexagon_Blue: return "highway hexagon blue";
  case RoadShieldType::Highway_Hexagon_Red: return "highway hexagon red";
  case RoadShieldType::Highway_Hexagon_Turkey: return "highway hexagon turkey";
  case RoadShieldType::US_Interstate: return "US interstate";
  case RoadShieldType::US_Highway: return "US highway";
  case RoadShieldType::UK_Highway: return "UK highway";
  case RoadShieldType::Bolivia_Fundamental: return "Bolivia fundamental";
  case RoadShieldType::Argentina_RN: return "Argentina national";
  case RoadShieldType::Brazil_National: return "Brazil national";
  case RoadShieldType::Brazil_State: return "Brazil state";
  case RoadShieldType::UY_National: return "UY national";
  case RoadShieldType::Italy_Autostrada: return "Italy autostrada";
  case RoadShieldType::Hungary_Green: return "hungary green";
  case RoadShieldType::Hungary_Blue: return "hungary blue";
  case RoadShieldType::Hidden: return "hidden";
  case RoadShieldType::Count: CHECK(false, ("RoadShieldType::Count is not to be used as a type"));
  }
  return std::string();
}

std::string DebugPrint(RoadShield const & shield)
{
  return DebugPrint(shield.m_type) + "/" + shield.m_name +
         (shield.m_additionalText.empty() ? "" : " (" + shield.m_additionalText + ")");
}
}  // namespace ftypes
