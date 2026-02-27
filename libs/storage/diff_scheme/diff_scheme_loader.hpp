#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/storage_defines.hpp"

#include <cstdint>
#include <functional>

#include "3party/ankerl/unordered_dense.h"

namespace storage
{
namespace diffs
{
struct LocalMapsInfo final
{
  using NameVersionMap = ankerl::unordered_dense::map<storage::CountryId, uint64_t>;

  uint64_t m_currentDataVersion = 0;
  NameVersionMap m_localMaps;
};

using DiffsReceivedCallback = std::function<void(diffs::NameDiffInfoMap && diffs)>;

class Loader final
{
public:
  static void Load(LocalMapsInfo && info, DiffsReceivedCallback && callback);
};
}  // namespace diffs
}  // namespace storage
