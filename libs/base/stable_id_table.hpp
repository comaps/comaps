#pragma once

#include <functional>
#include <map>

#include "base/assert.hpp"

namespace base
{
// ReSharper disable once CppTemplateParameterNeverUsed // contractual obligation
template <typename BigID, BigID const & Tombstone, typename LittleID, LittleID ReservedCount>
class StableIDTable
{
public:
  LittleID Remove(BigID const & bigID)
  {
    auto targetID = hash(bigID);

    auto currentTargetID = targetID;

    int tombstones = 0;
    do
    {
      auto currentBigID = m_hashTable.find(currentTargetID);

      ASSERT(currentBigID != m_hashTable.end(), ());

      if (currentBigID->second == bigID) [[likely]]
        break;

      if (currentBigID->second != Tombstone)
        tombstones = 0;
      else
        tombstones++;

      currentTargetID = incr(currentTargetID);
    }
    while (true);
    auto foundAtStableID = currentTargetID;

    // we need to leave a tombstone if the following elements contain any out-of-place elements
    // can't test hash(nextBigID->second) <= foundAtStableID because of wrapping
    currentTargetID = incr(currentTargetID);
    bool leaveTombstone = false;
    typename std::map<LittleID, BigID>::iterator nextBigID;
    while ((nextBigID = m_hashTable.find(currentTargetID)), nextBigID != m_hashTable.end()) [[unlikely]]
    {
      if (nextBigID->second != Tombstone)
      {
        auto nextTargetID = hash(nextBigID->second);
        if (nextTargetID != currentTargetID)
        {
          leaveTombstone = true;
          break;
        }
      }
      currentTargetID = incr(currentTargetID);
    }
    if (leaveTombstone) [[unlikely]]
      m_hashTable[foundAtStableID] = Tombstone;
    else [[likely]]
    {
      m_hashTable.erase(foundAtStableID);
      auto cleanUpStableID = foundAtStableID;
      // we can clean up the tombstones immediately preceding this element, too
      for (LittleID i = 0; i < tombstones; ++i) [[unlikely]]
      {
        cleanUpStableID = decr(cleanUpStableID);
        m_hashTable.erase(cleanUpStableID);
      }
    }
    return foundAtStableID;
  }

  LittleID GetStableID(BigID const & bigID) const
  {
    auto targetID = hash(bigID);

    auto currentTargetID = targetID;
    do
    {
      auto currentBigID = m_hashTable.find(currentTargetID);

      ASSERT(currentBigID != m_hashTable.end(), (currentTargetID, bigID));

      if (currentBigID->second == bigID) [[likely]]
        return currentTargetID;

      currentTargetID = incr(currentTargetID);
    }
    while (true);
  }

  LittleID EnsureStableID(BigID const & bigID)
  {
    auto targetID = hash(bigID);

    auto currentTargetID = targetID;
    do
    {
      auto currentBigID = m_hashTable.find(currentTargetID);

      if (currentBigID == m_hashTable.end()) [[unlikely]]
        break;

      if (currentBigID->second == bigID) [[likely]]
        return currentTargetID;

      currentTargetID = incr(currentTargetID);
    }
    while (true);

    // back to the start, hopefully we can overwrite a tombstone
    currentTargetID = targetID;
    do
    {
      auto currentBigID = m_hashTable.find(currentTargetID);
      if (currentBigID == m_hashTable.end() || currentBigID->second == Tombstone) [[likely]]
      {
        m_hashTable.insert_or_assign(currentTargetID, bigID);
        return currentTargetID;
      }

      // collision, try the next one
      currentTargetID = incr(currentTargetID);
    }
    while (true);
  }

  BigID const & At(LittleID littleID) const
  {
    BigID const & ret = m_hashTable.at(littleID);
    if (ret == Tombstone)
      throw std::out_of_range("There is a tombstone here");
    return ret;
  }

private:
  LittleID incr(LittleID id) const
  {
    auto constexpr max_id = std::numeric_limits<LittleID>::max();
    ASSERT(id >= ReservedCount, (id, ReservedCount));
    return id == max_id ? ReservedCount : id + 1;
  }

  LittleID decr(LittleID id) const
  {
    auto constexpr max_id = std::numeric_limits<LittleID>::max();
    ASSERT(id >= ReservedCount, (id, ReservedCount));
    return id == ReservedCount ? max_id : id - 1;
  }

  LittleID hash(BigID const & bigID) const
  {
    auto constexpr max_id = std::numeric_limits<LittleID>::max();
    static_assert(max_id < std::numeric_limits<size_t>::max());
    return static_cast<LittleID>(std::hash<BigID>()(bigID) % (max_id - ReservedCount + 1)) + ReservedCount;
  }

  std::map<LittleID, BigID> m_hashTable;
};
}  // namespace base
