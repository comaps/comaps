#pragma once

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"

#include "base/stl_helpers.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace platform
{
/**
 * @brief Represents a path to disk files corresponding to some country region.
 *
 * This class also wraps World.mwm and WorldCoasts.mwm files from resource bundle, when they can't
 * be found in a data directory. In this exceptional case, directory will be empty and
 * `SyncWithDisk()`/`DeleteFromDisk()`/`GetPath()`/`GetSize()` will return incorrect results.
 *
 * In any case, when you're going to read a file LocalCountryFile points to, use
 * `platform::GetCountryReader()`.
 */
class LocalCountryFile
{
public:
  LocalCountryFile();

  /**
   * @brief Creates an instance holding a path to countryFile's in a directory.
   *
   * Note that no disk operations are performed until `SyncWithDisk()` is called.
   *
   * @param directory full path to the country file
   * @param countryFile
   * @param version
   */
  LocalCountryFile(std::string directory, CountryFile countryFile, int64_t version);

  /**
   * @brief Syncs internal state like availability of files, their sizes etc. with disk.
   *
   * Generality speaking it's not always true. To know it for sure it's necessary to read a mwm in
   * this method but it's not implemented by performance reasons. This check is done on
   * building routes stage.
   */
  void SyncWithDisk();

  /**
   * @brief Deletes a file from disk.
   *
   * Removes the specified file from disk for `LocalCountryFile`, if it is known, i.e. it was found
   * by a previous SyncWithDisk() call.
   * @param type
   */
  void DeleteFromDisk(MapFileType type) const;

  /**
   * @brief Returns the path to a file.
   *
   * Return value may be empty until SyncWithDisk() is called.
   *
   * @param type
   * @return
   */
  std::string GetPath(MapFileType type) const;
  std::string GetFileName(MapFileType type) const;

  /**
   * @brief Returns the size of a file.
   *
   * Return value may be zero until SyncWithDisk() is called.
   *
   * @param type
   * @return
   */
  uint64_t GetSize(MapFileType type) const;

  /**
   * @brief Returns true when files are found during `SyncWithDisk()`.
   *
   * Return value is false until `SyncWithDisk()` is called.
   *
   * @return true if files were found, false if no files were found, or before the first call to `SyncWithDisk()`
   */
  bool HasFiles() const;

  /**
   * @brief Checks whether files specified in filesMask are on disk.
   *
   * Return value will be false until SyncWithDisk() is called.
   *
   * @param type
   * @return
   */
  bool OnDisk(MapFileType type) const;

  bool IsInBundle() const { return m_directory.empty(); }
  std::string const & GetDirectory() const { return m_directory; }
  std::string const & GetCountryName() const { return m_countryFile.GetName(); }
  int64_t GetVersion() const { return m_version; }
  CountryFile const & GetCountryFile() const { return m_countryFile; }

  bool operator<(LocalCountryFile const & rhs) const;
  bool operator==(LocalCountryFile const & rhs) const;
  bool operator!=(LocalCountryFile const & rhs) const { return !(*this == rhs); }

  bool ValidateIntegrity() const;

  /**
   * @brief Creates a `LocalCountryFile` for test purposes.
   *
   * Creates a `LocalCountryFile` for test purposes, for a country region with `countryFileName`.
   * Automatically performs sync with disk.
   *
   * @param countryFileName The filename, without any extension.
   * @param version The data version.
   * @return
   */
  static LocalCountryFile MakeForTesting(std::string countryFileName, int64_t version = 0);

  // Used in generator only to simplify getting instance from path.
  static LocalCountryFile MakeTemporary(std::string const & fullPath);

private:
  friend std::string DebugPrint(LocalCountryFile const &);
  friend void FindAllLocalMapsAndCleanup(int64_t latestVersion, std::string const & dataDir,
                                         std::vector<LocalCountryFile> & localFiles);

  /// Can be bundled (empty directory) or path to the file.
  std::string m_directory;
  CountryFile m_countryFile;
  int64_t m_version;

  using File = std::optional<uint64_t>;
  std::array<File, base::Underlying(MapFileType::Count)> m_files = {};
};

std::string DebugPrint(LocalCountryFile const & file);
}  // namespace platform
