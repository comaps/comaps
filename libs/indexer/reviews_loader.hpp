#pragma once

#include "indexer/reviews_model.hpp"
#include "indexer/reviews_serdes.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include <map>
#include <memory>
#include <mutex>

class DataSource;

namespace reviews
{

class Loader
{
public:
  explicit Loader(DataSource const & dataSource) : m_dataSource(dataSource) {}

  std::optional<FeatureReviews> GetReviews(FeatureID const & featureId);

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
} // namespace reviews
