#include "indexer/reviews_loader.hpp"

#include "indexer/data_source.hpp"

#include "base/assert.hpp"

#include "defines.hpp"

namespace reviews
{
std::optional<FeatureReviews> Loader::GetReviews(FeatureID const & featureId)
{
  auto const handle = m_dataSource.GetMwmHandleById(featureId.m_mwmId);

  if (!handle.IsAlive())
    return {};

  auto const & value = *handle.GetValue();

  if (!value.m_cont.IsExist(REVIEWS_FILE_TAG))
    return {};

  EntryPtr entry;
  {
    std::lock_guard lock(m_mutex);
    if (auto const it = m_deserializers.find(featureId.m_mwmId); it != m_deserializers.end())
      entry = it->second;
    else
    {
      entry = std::make_shared<Entry>();
      m_deserializers[featureId.m_mwmId] = entry;
    }
  }

  ASSERT(entry, ());

  auto const readerPtr = value.m_cont.GetReader(REVIEWS_FILE_TAG);

  std::lock_guard lock(entry->m_mutex);
  return entry->m_deserializer.Deserialize(*readerPtr.GetPtr(), featureId.m_index);
}
}  // namespace reviews
