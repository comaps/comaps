#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace editor
{

struct OSMTag
{
  std::string key;
  std::string value;
};

class TypeToOSMTranslator
{
public:
  std::vector<OSMTag> static constexpr kEmptyResult = {};

  TypeToOSMTranslator() : TypeToOSMTranslator(true) {}
  explicit TypeToOSMTranslator(bool initialize);

  void LoadConfigFile();
  void LoadFromStream(std::istream & s);
  std::vector<OSMTag> const & OsmTagsFromType(uint32_t type) const;

  std::unordered_map<uint32_t, std::vector<OSMTag>> const & GetStorage() const { return m_storage; }

private:
  std::unordered_map<uint32_t, std::vector<OSMTag>> m_storage;
};

TypeToOSMTranslator const & GetOSMTranslator();

}  // namespace editor
