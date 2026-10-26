#include "editor/changeset_wrapper.hpp"

#include "editor/osm_auth.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <exception>
#include <sstream>
#include <utility>

namespace
{
std::string_view constexpr kVowels = "aeiouy";

std::string_view constexpr kMainTags[] = {"amenity", "shop",     "tourism",  "historic",   "craft",     "emergency",
                                          "barrier", "highway",  "office",   "leisure",    "waterway",  "natural",
                                          "place",   "entrance", "man_made", "healthcare", "attraction"};

std::string GetTypeForFeature(editor::XMLFeature const & node)
{
  for (std::string_view const key : kMainTags)
  {
    if (node.HasTag(key))
    {
      // Non-const for RVO.
      std::string value = node.GetTagValue(key);
      if (value == "yes")
        return std::string{key};
      else if (key == "shop" || key == "office" || key == "entrance" || key == "attraction")
        return value.append(" ").append(key);  // "convenience shop"
      else if (!value.empty() && value.back() == 's')
        // Remove 's' from the tail: "toilets" -> "toilet".
        return value.erase(value.size() - 1);
      else if (key == "healthcare" && value == "alternative")
        return "alternative medicine";
      return value;
    }
  }

  if (node.HasTag("disused:shop") || node.HasTag("disused:amenity"))
    return "vacant business";

  if (node.HasTag("building"))
  {
    std::string value = node.GetTagValue("building");
    return value == "yes" ? "building" : value.append(" building");
  }

  if (node.HasTag("addr:housenumber") || node.HasTag("addr:street") || node.HasTag("addr:postcode"))
    return "address";

  // Did not find any known tags.
  return node.HasAnyTags() ? "unknown object" : "empty object";
}

}  // namespace

namespace pugi
{
std::string DebugPrint(xml_document const & doc)
{
  std::ostringstream stream;
  doc.print(stream, "  ");
  return stream.str();
}
}  // namespace pugi

namespace osm
{
ChangesetWrapper::ChangesetWrapper(std::string const & keySecret, ServerApi06::KeyValueTags comments) noexcept
  : m_changesetComments(std::move(comments))
  , m_api(OsmOAuth::ServerAuth(keySecret))
{}

ChangesetWrapper::~ChangesetWrapper()
{
  if (m_changesetId)
  {
    try
    {
      AddChangesetTag("comment", GetDescription());
      m_api.UpdateChangeSet(m_changesetId, m_changesetComments);
      m_api.CloseChangeSet(m_changesetId);
    }
    catch (std::exception const & ex)
    {
      LOG(LWARNING, (ex.what()));
    }
  }
}

void ChangesetWrapper::Create(editor::XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  // TODO(AlexZ): Think about storing/logging returned OSM ids.
  UNUSED_VALUE(m_api.CreateElement(node));
  m_created_types[GetTypeForFeature(node)]++;
}

void ChangesetWrapper::Modify(editor::XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  m_api.ModifyElement(node);
  m_modified_types[GetTypeForFeature(node)]++;
}

void ChangesetWrapper::AddChangesetTag(std::string key, std::string value)
{
  // Truncate to 254 characters as OSM has a length limit of 255
  if (strings::Truncate(value, kMaximumOsmChars))
    value += "…";

  value = strings::EscapeForXML(value);

  m_changesetComments.insert_or_assign(std::move(key), std::move(value));
}

void ChangesetWrapper::AddToChangesetKeyList(std::string key, std::string value)
{
  auto it = m_changesetComments.find(key);
  if (it == m_changesetComments.end())
    AddChangesetTag(std::move(key), std::move(value));
  else
    AddChangesetTag(std::move(key), it->second + "; " + value);
}

void ChangesetWrapper::Delete(editor::XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  m_api.DeleteElement(node);
  m_deleted_types[GetTypeForFeature(node)]++;
}

std::string ChangesetWrapper::TypeCountToString(TypeCount const & typeCount)
{
  if (typeCount.empty())
    return {};

  // Convert map to vector and sort pairs by count, descending.
  std::vector<std::pair<std::string, size_t>> items{typeCount.begin(), typeCount.end()};

  sort(items.begin(), items.end(), [](auto const & a, auto const & b) { return a.second > b.second; });

  std::ostringstream ss;
  size_t const limit = std::min(size_t(3), items.size());
  for (size_t i = 0; i < limit; ++i)
  {
    if (i > 0)
    {
      // Separator: "A and B" for two, "A, B, and C" for three or more.
      if (limit > 2)
        ss << ", ";
      else
        ss << " ";
      if (i == limit - 1)
        ss << "and ";
    }

    auto & currentPair = items[i];
    // If we have more objects left, make the last one a list of these.
    if (i == limit - 1 && limit < items.size())
    {
      size_t count = 0;
      for (auto j = i; j < items.size(); ++j)
        count += items[j].second;
      currentPair = {"other object", count};
    }

    // Format a count: "a shop" for single shop, "4 shops" for multiple.
    if (currentPair.second == 1)
      if (kVowels.find(currentPair.first.front()) != std::string::npos)
        ss << "an";
      else
        ss << "a";
    else
      ss << currentPair.second;
    ss << ' ' << currentPair.first;
    if (currentPair.second > 1)
    {
      if (currentPair.first.size() >= 2)
      {
        std::string const lastTwo = currentPair.first.substr(currentPair.first.size() - 2);
        // "bench" -> "benches", "marsh" -> "marshes", etc.
        if (lastTwo.back() == 'x' || lastTwo == "sh" || lastTwo == "ch" || lastTwo == "ss")
        {
          ss << 'e';
        }
        // "library" -> "libraries"
        else if (lastTwo.back() == 'y' && kVowels.find(lastTwo.front()) == std::string::npos)
        {
          ss.seekp(ss.tellp() - std::ostringstream::pos_type{1});
          ss << "ie";
        }
      }
      ss << 's';
    }
  }
  return ss.str();
}

std::string ChangesetWrapper::GetDescription() const
{
  std::string result;
  if (!m_created_types.empty())
    result.append("Created ").append(TypeCountToString(m_created_types));
  if (!m_modified_types.empty())
  {
    if (!result.empty())
      result.append("; ");
    result.append("Updated ").append(TypeCountToString(m_modified_types));
  }
  if (!m_deleted_types.empty())
  {
    if (!result.empty())
      result.append("; ");
    result.append("Deleted ").append(TypeCountToString(m_deleted_types));
  }
  return result;
}
}  // namespace osm
