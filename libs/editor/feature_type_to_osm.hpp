#pragma once

#include "indexer/osm_element.hpp"

#include <cstdint>
#include <vector>

#include "3party/ankerl/unordered_dense.h"

namespace editor
{

class TypeToOSMTranslator
{
public:
  std::vector<OsmElement::Tag> static constexpr kEmptyResult = {};

  TypeToOSMTranslator() : TypeToOSMTranslator(true) {}
  explicit TypeToOSMTranslator(bool initialize);

  void LoadConfigFile();
  void LoadFromStream(std::istream & s);
  std::vector<OsmElement::Tag> const & OsmTagsFromType(uint32_t type) const;

  ankerl::unordered_dense::map<uint32_t, std::vector<OsmElement::Tag>> const & GetStorage() const { return m_storage; }

private:
  ankerl::unordered_dense::map<uint32_t, std::vector<OsmElement::Tag>> m_storage;
};

TypeToOSMTranslator const & GetOSMTranslator();

}  // namespace editor
