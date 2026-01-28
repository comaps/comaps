#include "editor/feature_type_to_osm.hpp"

#include "base/assert.hpp"
#include "coding/reader_streambuf.hpp"
#include "indexer/classificator.hpp"
#include "platform/platform.hpp"

#include <string>

namespace editor
{
TypeToOSMTranslator::TypeToOSMTranslator(bool initialize)
{
  if (initialize)
    LoadConfigFile();
}

void TypeToOSMTranslator::LoadConfigFile()
{
  Platform & p = GetPlatform();
  std::unique_ptr<ModelReader> reader = p.GetReader("mapcss-mapping.csv");
  ReaderStreamBuf buffer(std::move(reader));
  std::istream s(&buffer);

  LoadFromStream(s);
}

void TypeToOSMTranslator::LoadFromStream(std::istream & s)
{
  m_storage.clear();

  std::string line;
  while (s.good())
  {
    getline(s, line);
    strings::Trim(line);

    // skip empty lines and comments
    if (line.empty() || line.front() == '#')
      continue;

    std::vector<std::string> rowTokens;
    strings::ParseCSVRow(line, ';', rowTokens);

    // make sure entry is in full or short format
    if (rowTokens.size() != 3 && rowTokens.size() != 7)
    {
      ASSERT(false, ("Invalid feature type definition:", line));
      continue;
    }

    // skip deprecated and moved types
    if ((rowTokens.size() == 3 && !rowTokens[2].empty()) || (rowTokens.size() == 7 && rowTokens[2] == "x"))
      continue;

    // Get internal feature type
    ASSERT(!rowTokens[0].empty(), ("No feature type found:", line));
    std::vector<std::string_view> const featureTypeTokens = strings::Tokenize(rowTokens[0], "|");
    uint32_t const type = classif().GetTypeByPathSafe(featureTypeTokens);
    ASSERT(type != IndexAndTypeMapping::INVALID_TYPE, ("Feature with invalid type:", line));

    if (rowTokens.size() == 3)
    {
      // Derive OSM tags from type name
      ASSERT(featureTypeTokens.size() <= 2, ("OSM tags can not be inferred from name:", line));

      OSMTag osmTag;

      // e.g. "amenity-restaurant"
      if (featureTypeTokens.size() >= 2)
      {
        osmTag.key = featureTypeTokens[0];
        osmTag.value = featureTypeTokens[1];
      }
      // e.g. "building"
      else if (featureTypeTokens.size() == 1)
      {
        osmTag.key = featureTypeTokens[0];
        osmTag.value = "yes";
      }

      m_storage.insert({type, {osmTag}});
    }
    else
    {
      // OSM tags are listed in the feature type entry
      if (rowTokens[1].empty())
      {
        ASSERT(false, ("No OSM tags found:", line));
        continue;
      }
      std::vector<std::string_view> const osmTagTokens = strings::Tokenize(rowTokens[1], ",");

      // First entry is the best practice way to tag a feature
      std::string_view const osmTagList = osmTagTokens[0];

      // Process OSM tag list (e.g. "[tourism=information][information=office]")
      std::vector<OSMTag> osmTags;
      size_t pos = 0;

      while ((pos = osmTagList.find('[', pos)) != std::string::npos)
      {
        size_t end = osmTagList.find(']', pos);

        if (end == std::string::npos)
        {
          ASSERT(false, ("Bracket not closed in OSM tag:", line));
          break;
        }

        std::string_view keyValuePair = osmTagList.substr(pos + 1, end - pos - 1);

        if (keyValuePair.empty())
        {
          ASSERT(false, ("Key value pair is empty:", line));
          break;
        }

        size_t equalSign = keyValuePair.find('=');
        if (equalSign != std::string::npos)
        {
          // Tags in key=value format
          OSMTag osmTag;
          osmTag.key = keyValuePair.substr(0, equalSign);
          osmTag.value = keyValuePair.substr(equalSign + 1);

          // mapcss-mapping.csv uses 'not' instead of 'no' as a workaround for the rendering engine
          if (osmTag.value == "not")
            osmTag.value = "no";

          osmTags.push_back(osmTag);
        }
        else if (keyValuePair.front() == '!')
        {
          // Tags with "forbidden" selector '!' are skipped
        }
        else
        {
          // Tags with optional "mandatory" selector '?'
          if (keyValuePair.back() == '?')
            keyValuePair.remove_suffix(1);

          OSMTag osmTag;
          osmTag.key = keyValuePair;
          osmTag.value = "yes";

          osmTags.push_back(osmTag);
        }

        pos = end + 1;
      }

      ASSERT(!osmTags.empty(), ("No OSM tags found for feature:", line));

      m_storage.insert({type, osmTags});
    }
  }
}

std::vector<OSMTag> const & TypeToOSMTranslator::OsmTagsFromType(uint32_t type) const
{
  auto it = m_storage.find(type);

  if (it == m_storage.end())
  {
    ASSERT(false, ("OSM tags for type", type, "could not be found"));
    return kEmptyResult;
  }

  return it->second;
}

TypeToOSMTranslator const & GetOSMTranslator()
{
  static TypeToOSMTranslator translator;
  return translator;
}
}  // namespace editor
