#pragma once

#include "platform/local_country_file.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "3party/ankerl/unordered_dense.h"

namespace storage
{
using CountryId = std::string;
using CountriesSet = std::set<CountryId>;
using CountriesVec = std::vector<CountryId>;
using LocalFilePtr = std::shared_ptr<platform::LocalCountryFile>;
using OldMwmMapping = std::map<CountryId, CountriesSet>;
/// Map from key affiliation words into CountryIds.
using Affiliations = ankerl::unordered_dense::map<std::string, std::vector<CountryId>>;
/// Map from country name synonyms and old names into CountryId.
using CountryNameSynonyms = ankerl::unordered_dense::map<std::string, CountryId>;
/// Map from CountryId into city GeoObject id.
using MwmTopCityGeoIds = ankerl::unordered_dense::map<CountryId, base::GeoObjectId>;
using MwmTopCountryGeoIds = ankerl::unordered_dense::map<CountryId, std::vector<base::GeoObjectId>>;

extern storage::CountryId const kInvalidCountryId;

// @TODO(bykoianko) Check in country tree if the countryId is valid.
bool IsCountryIdValid(CountryId const & countryId);

/// Inner status which is used inside Storage class
enum class Status : uint8_t
{
  Undefined = 0,
  OnDisk,          /**< Downloaded mwm(s) is up to date. No need to update it. */
  NotDownloaded,   /**< Mwm can be download but not downloaded yet. */
  DownloadFailed,  /**< Downloading failed because no internet connection. */
  Downloading,     /**< Downloading a new mwm or updating an old one. */
  Applying,        /**< Applying downloaded diff for an old mwm. */
  InQueue,         /**< A mwm is waiting for downloading in the queue. */
  UnknownError,    /**< Downloading failed because of unknown error. */
  OnDiskOutOfDate, /**< An update for a downloaded mwm is ready according to counties.txt. */
  OutOfMemFailed,  /**< Downloading failed because it's not enough memory */
};
std::string DebugPrint(Status status);

/// \note The order of enum items is important. It is used in Storage::NodeStatus method.
/// If it's necessary to add more statuses it's better to add to the end.
enum class NodeStatus
{
  Undefined,
  Downloading,     /**< Downloading a new mwm or updating an old one. */
  Applying,        /**< Applying downloaded diff for an old mwm. */
  InQueue,         /**< An mwm is waiting for downloading in the queue. */
  Error,           /**< An error happened while downloading */
  OnDiskOutOfDate, /**< An update for a downloaded mwm is ready according to counties.txt. */
  OnDisk,          /**< Downloaded mwm(s) is up to date. No need to update it. */
  NotDownloaded,   /**< An mwm can be downloaded but not downloaded yet. */
  Partly,          /**< Leafs of group node has a mix of NotDownloaded and OnDisk status. */
};
std::string DebugPrint(NodeStatus status);

enum class NodeErrorCode
{
  NoError,
  UnknownError,     /**< Downloading failed because of unknown error. */
  OutOfMemFailed,   /**< Downloading failed because it's not enough memory */
  NoInetConnection, /**< Downloading failed because internet connection was interrupted */
};
std::string DebugPrint(NodeErrorCode status);

/// \note The order of enum items is important, they're mirrored in MapManager.java
enum class CheckUpdatesStatus
{
  Undefined,
  Updated,   /**< An update had been pulled and applied successfully */
  NoUpdate,  /**< No updates available at the moment */
  EOL,       /**< No more updates planned for app's map series */
  Error,     /**< An error happened while checking */
};
std::string DebugPrint(CheckUpdatesStatus status);

struct StatusAndError
{
  StatusAndError(NodeStatus nodeStatus, NodeErrorCode nodeError) : status(nodeStatus), error(nodeError) {}

  bool operator==(StatusAndError const & other) const { return other.status == status && other.error == error; }

  NodeStatus status;
  NodeErrorCode error;
};
std::string DebugPrint(StatusAndError statusAndError);

StatusAndError ParseStatus(Status innerStatus);
}  // namespace storage

using DownloadFn = std::function<void(storage::CountryId const &)>;
