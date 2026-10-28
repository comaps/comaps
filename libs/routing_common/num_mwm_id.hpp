#pragma once

#include "platform/country_file.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <limits>
#include <map>
#include <vector>

namespace routing
{
using NumMwmId = std::uint16_t;
NumMwmId constexpr kFakeNumMwmId = std::numeric_limits<NumMwmId>::max();
NumMwmId constexpr kGeneratorMwmId = 0;

/**
 * @brief A numbered list of country files.
 */
class NumMwmIds final
{
public:
  bool IsEmpty() const { return m_idToFile.empty(); }

  /**
   * @brief Registers a file, i.e. adds it to the instance.
   *
   * If the instance already contains the file, this is a no-op.
   *
   * @param file
   */
  void RegisterFile(platform::CountryFile const & file)
  {
    if (ContainsFile(file))
      return;

    NumMwmId const id = base::asserted_cast<NumMwmId>(m_idToFile.size());
    m_idToFile.push_back(file);
    m_fileToId[file] = id;

    // LOG(LDEBUG, ("MWM:", file.GetName(), "=", id));
  }

  /**
   * @brief Whether this instance contains a given file.
   * @param file
   * @return
   */
  bool ContainsFile(platform::CountryFile const & file) const { return m_fileToId.find(file) != m_fileToId.cend(); }

  /**
   * @brief Whether this instance contains a file at a given index.
   * @param mwmId The index.
   * @return
   */
  bool ContainsFileForMwm(NumMwmId mwmId) const { return mwmId < m_idToFile.size(); }

  /**
   * @brief Returns a file by index.
   * @param mwmId The index.
   * @return
   */
  platform::CountryFile const & GetFile(NumMwmId mwmId) const
  {
    ASSERT_LESS(mwmId, m_idToFile.size(), ());
    return m_idToFile[mwmId];
  }

  /**
   * @brief Returns the index for a given file.
   * @param file
   * @return
   */
  NumMwmId GetId(platform::CountryFile const & file) const
  {
    auto const it = m_fileToId.find(file);
    ASSERT(it != m_fileToId.cend(), ("Can't find mwm id for", file));
    return it->second;
  }

  template <typename F>
  void ForEachId(F && f) const
  {
    for (NumMwmId id = 0; id < base::asserted_cast<NumMwmId>(m_idToFile.size()); ++id)
      f(id);
  }

private:
  std::vector<platform::CountryFile> m_idToFile;
  std::map<platform::CountryFile, NumMwmId> m_fileToId;
};
}  //  namespace routing
