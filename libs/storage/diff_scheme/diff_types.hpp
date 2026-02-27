#pragma once

#include "storage/storage_defines.hpp"

#include <cstdint>
#include <string>

#include "3party/ankerl/unordered_dense.h"

namespace storage
{
namespace diffs
{
// Status of the diffs data source as a whole.
enum class Status
{
  NotAvailable,
  Available
};

struct DiffInfo final
{
  DiffInfo(uint64_t size, uint64_t version) : m_size(size), m_version(version) {}

  uint64_t m_size;
  uint64_t m_version;
  bool m_isApplied = false;
};

using NameDiffInfoMap = ankerl::unordered_dense::map<storage::CountryId, DiffInfo>;
}  // namespace diffs
}  // namespace storage
