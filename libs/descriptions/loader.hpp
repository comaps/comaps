#pragma once

#include "descriptions/serdes.hpp"

#include "indexer/mwm_set.hpp"

#include "i18n/localisation.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class DataSource;
struct FeatureID;

namespace descriptions
{
// *NOTE* This class IS thread-safe.
class Loader
{
public:
  explicit Loader(DataSource const & dataSource) : m_dataSource(dataSource) {}

  std::string GetWikiDescription(FeatureID const & featureId, std::vector<localisation::LanguageIndex> const & prioritizedLanguageIndexes);

private:
  struct Entry
  {
    std::mutex m_mutex;
    Deserializer m_deserializer;
  };

  using EntryPtr = std::shared_ptr<Entry>;

  DataSource const & m_dataSource;
  std::map<MwmSet::MwmId, EntryPtr> m_deserializers;
  std::mutex m_mutex;
};
}  // namespace descriptions
