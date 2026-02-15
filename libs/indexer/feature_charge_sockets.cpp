
#include "indexer/feature_charge_sockets.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <iomanip>  // for formatting doubles
#include <sstream>  // for std::to_string alternative
#include <string_view>
#include <vector>

// helper to format doubles (avoids trailing zeros)
inline std::string to_string_trimmed(double value, int precision = 2)
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  auto str = oss.str();

  // Remove trailing zeros
  auto pos = str.find('.');
  if (pos != std::string::npos)
  {
    // erase trailing zeros
    while (!str.empty() && str.back() == '0')
      str.pop_back();
    // if we end with a '.', remove it too
    if (!str.empty() && str.back() == '.')
      str.pop_back();
  }
  return str;
}

// we use a custom tokenizer, as strings::Tokenize discards empty
// tokens and we want to keep them:
//
// Example:
//    "a||" → ["a", "", ""]
//    "b|0|" → ["b", "0", ""]
//    "c||34" → ["c", "", "34"]
//    "d|43|54" → ["d", "43", "54"]
std::vector<std::string> tokenize(std::string_view s, char delim = '|')
{
  std::vector<std::string> tokens;
  size_t start = 0;
  while (true)
  {
    size_t pos = s.find(delim, start);
    if (pos == std::string_view::npos)
    {
      tokens.emplace_back(s.substr(start));
      break;
    }
    tokens.emplace_back(s.substr(start, pos - start));
    start = pos + 1;
  }
  return tokens;
}

ChargeSocketsHelper::ChargeSocketsHelper(std::string const & socketsList) : m_dirty(false)
{
  if (socketsList.empty())
    return;

  auto tokens = tokenize(socketsList, ';');

  for (auto token : tokens)
  {
    if (token.empty())
      continue;

    auto fields = tokenize(token, '|');

    if (fields.size() != 3)
      continue;  // invalid entry, skip

    ChargeSocketDescriptor desc;

    if (std::find(SUPPORTED_TYPES.begin(), SUPPORTED_TYPES.end(), fields[0]) != SUPPORTED_TYPES.end())
      desc.type = fields[0];
    else
      desc.type = UNKNOWN;

    try
    {
      desc.count = std::stoi(std::string(fields[1]));
    }
    catch (...)
    {
      desc.count = 0;
    }

    try
    {
      desc.power = std::stod(std::string(fields[2]));
    }
    catch (...)
    {
      desc.power = 0;
    }
    m_chargeSockets.push_back(desc);
  }
  m_dirty = true;
}

void ChargeSocketsHelper::AddSocket(std::string const & type, unsigned int count, double power)
{
  if (power < 0)
  {
    LOG(LWARNING, ("Invalid socket power. Must be >= 0:", power));
    return;
  }

  m_chargeSockets.push_back({type, count, power});
  m_dirty = true;
}

void ChargeSocketsHelper::AggregateChargeSocketKey(std::string const & k, std::string const & v)
{
  auto keys = strings::Tokenize(k, ":");
  ASSERT(keys[0] == "socket", ());  // key must start with "socket:"
  if (keys.size() < 2 || keys.size() > 3)
  {
    LOG(LWARNING, ("Invalid socket key:", k));
    return;
  }

  std::string type(keys[1]);

  bool isOutput = false;
  if (keys.size() == 3)
  {
    if (keys[2] == "output")
      isOutput = true;
    else
      return;  // ignore other suffixes
  }

  // normalize type
  static std::unordered_map<std::string_view, std::string_view> const kTypeMap = {
      {"tesla_supercharger", "nacs"},
      {"tesla_destination", "nacs"},
      {"tesla_standard", "nacs"},
      {"tesla", "nacs"},
      {"tesla_supercharger_ccs", "type2_combo"},
      {"ccs", "type2_combo"},
      {"type1_cable", "type1"},
  };

  auto itMap = kTypeMap.find(type);
  if (itMap != kTypeMap.end())
    type = itMap->second;

  if (std::find(SUPPORTED_TYPES.begin(), SUPPORTED_TYPES.end(), type) == SUPPORTED_TYPES.end())
    type = UNKNOWN;

  // Tokenize counts or powers
  auto tokens = strings::Tokenize(v, ";");
  if (tokens.empty())
    return;

  if (!isOutput)
  {
    // Parse counts
    std::vector<unsigned int> counts;
    for (auto & t : tokens)
    {
      strings::Trim(t);
      if (t.empty())
        continue;

      if (t == "yes")
        counts.push_back(0);
      else
      {
        try
        {
          auto c = std::stoi(std::string(t));
          if (c > 0)
            counts.push_back(c);
          else
          {
            LOG(LWARNING, ("Invalid count. Ignoring:", t));
            continue;
          }
        }
        catch (...)
        {
          LOG(LWARNING, ("Invalid count. Ignoring:", t));
          continue;
        }
      }
    }

    // Update existing sockets or add new ones
    for (size_t i = 0; i < counts.size(); ++i)
    {
      // Find the i-th socket of this type
      size_t matched = 0;
      auto it = std::find_if(m_chargeSockets.begin(), m_chargeSockets.end(), [&](ChargeSocketDescriptor const & d)
      {
        if (d.type != type)
          return false;
        if (matched == i)
          return true;
        ++matched;
        return false;
      });

      if (it != m_chargeSockets.end())
        it->count = counts[i];
      else
        m_chargeSockets.push_back({type, counts[i], 0});
    }
  }
  else
  {
    // Parse powers
    std::vector<double> powers;
    for (auto & t : tokens)
    {
      strings::Trim(t);
      if (t.empty())
        continue;

      std::string s = strings::MakeLowerCase(std::string(t));
      size_t pos = s.find("va");
      while (pos != std::string::npos)
      {
        s.replace(pos, 2, "w");
        pos = s.find("va", pos + 1);
      }

      double value = 0;
      enum PowerUnit
      {
        WATT,
        KILOWATT,
        MEGAWATT
      } unit = KILOWATT;

      if (!s.empty() && s.back() == 'w')
      {
        unit = WATT;
        s.pop_back();
        if (!s.empty() && s.back() == 'k')
        {
          unit = KILOWATT;
          s.pop_back();
        }
        else if (!s.empty() && s.back() == 'm')
        {
          unit = MEGAWATT;
          s.pop_back();
        }
      }

      try
      {
        value = std::stod(s);
        if (value < 0)
        {
          LOG(LWARNING, ("Invalid power value. Ignoring:", t));
          continue;
        }
        switch (unit)
        {
        case WATT: value /= 1000.; break;
        case MEGAWATT: value *= 1000.; break;
        case KILOWATT: break;
        }
      }
      catch (...)
      {
        LOG(LWARNING, ("Invalid power value. Ignoring:", t));
        continue;
      }

      powers.push_back(value);
    }
    // Update existing sockets or add new ones
    for (size_t i = 0; i < powers.size(); ++i)
    {
      size_t matched = 0;
      auto it = std::find_if(m_chargeSockets.begin(), m_chargeSockets.end(), [&](ChargeSocketDescriptor const & d)
      {
        if (d.type != type)
          return false;
        if (matched == i)
          return true;
        ++matched;
        return false;
      });

      if (it != m_chargeSockets.end())
        it->power = powers[i];
      else
        m_chargeSockets.push_back({type, 0, powers[i]});  // default count=0
    }
  }

  m_dirty = true;
}

ChargeSocketDescriptors ChargeSocketsHelper::GetSockets()
{
  if (m_dirty)
    Sort();

  return m_chargeSockets;
}

std::string ChargeSocketsHelper::ToString()
{
  if (m_dirty)
    Sort();

  std::ostringstream oss;

  for (size_t i = 0; i < m_chargeSockets.size(); ++i)
  {
    auto const & desc = m_chargeSockets[i];

    oss << desc.type << "|";
    if (desc.count > 0)
      oss << desc.count;
    oss << "|";
    if (desc.power > 0)
      oss << desc.power;

    if (i + 1 < m_chargeSockets.size())
      oss << ";";
  }
  return oss.str();
}

OSMKeyValues ChargeSocketsHelper::GetOSMKeyValues()
{
  if (m_dirty)
    Sort();

  std::vector<std::pair<std::string, std::string>> result;
  std::string lastType;
  std::ostringstream countStream;
  std::ostringstream powerStream;
  bool firstCount = true;
  bool firstPower = true;

  for (auto const & s : m_chargeSockets)
  {
    if (s.type.empty())
      continue;

    // If we switch type, flush previous streams
    if (s.type != lastType && !lastType.empty())
    {
      result.emplace_back("socket:" + lastType, countStream.str());
      if (powerStream.str().size() > 0)
        result.emplace_back("socket:" + lastType + ":output", powerStream.str());

      countStream.str("");
      countStream.clear();
      powerStream.str("");
      powerStream.clear();
      firstCount = true;
      firstPower = true;
    }

    lastType = s.type;

    // Append count
    if (!firstCount)
      countStream << ";";
    countStream << ((s.count > 0) ? std::to_string(s.count) : "yes");
    firstCount = false;

    // Append power if > 0
    if (s.power > 0)
    {
      if (!firstPower)
        powerStream << ";";
      powerStream << to_string_trimmed(s.power);
      firstPower = false;
    }
  }

  // Flush last type
  if (!lastType.empty())
  {
    result.emplace_back("socket:" + lastType, countStream.str());
    if (powerStream.str().size() > 0)
      result.emplace_back("socket:" + lastType + ":output", powerStream.str());
  }

  return result;
}

KeyValueDiffSet ChargeSocketsHelper::Diff(ChargeSocketDescriptors const & oldSockets)
{
  KeyValueDiffSet result;

  auto oldKeys = ChargeSocketsHelper(oldSockets).GetOSMKeyValues();
  auto newKeys = GetOSMKeyValues();

  std::unordered_map<std::string, std::string> oldMap, newMap;

  for (auto && [k, v] : oldKeys)
    oldMap.emplace(k, v);
  for (auto && [k, v] : newKeys)
    newMap.emplace(k, v);

  // Handle keys that exist in old
  for (auto && [k, oldVal] : oldMap)
    if (auto it = newMap.find(k); it == newMap.end())
      result.insert({k, oldVal, ""});  // deleted
    else if (it->second != oldVal)
      result.insert({k, oldVal, it->second});  // updated

  // Handle keys that are only new
  for (auto && [k, newVal] : newMap)
    if (!oldMap.contains(k))
      result.insert({k, "", newVal});  // created

  return result;
}

void ChargeSocketsHelper::Sort()
{
  size_t const unknownTypeOrder = SUPPORTED_TYPES.size();

  std::sort(m_chargeSockets.begin(), m_chargeSockets.end(),
            [&](ChargeSocketDescriptor const & a, ChargeSocketDescriptor const & b)
  {
    auto const itA = std::find(SUPPORTED_TYPES.begin(), SUPPORTED_TYPES.end(), a.type);
    auto const orderA = (itA == SUPPORTED_TYPES.end()) ? unknownTypeOrder : std::distance(SUPPORTED_TYPES.begin(), itA);
    auto const itB = std::find(SUPPORTED_TYPES.begin(), SUPPORTED_TYPES.end(), b.type);
    auto const orderB = (itB == SUPPORTED_TYPES.end()) ? unknownTypeOrder : std::distance(SUPPORTED_TYPES.begin(), itB);

    if (orderA != orderB)
      return orderA < orderB;
    return a.power > b.power;  // Sort by power in descending order for sockets of the same type
  });

  m_dirty = false;
}
