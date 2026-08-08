#pragma once

#include "storage/downloader_queue_universal.hpp"
#include "storage/storage_defines.hpp"

#include "platform/http_request.hpp"
#include "platform/safe_callback.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "platform/servers_list.hpp"

class DownloadingPolicy;

namespace storage
{
using downloader::MetaConfig;

// This interface encapsulates HTTP routines for receiving servers
// URLs and downloading a single map file.
class MapFilesDownloader
{
public:
  // Denotes bytes downloaded and total number of bytes.
  using ServersList = std::vector<std::string>;
  using MetaConfigCallback = platform::SafeCallback<void(MetaConfig const & metaConfig)>;

  virtual ~MapFilesDownloader() = default;

  /// Asynchronously downloads a map file
  /// (first Checks for Updates and queries metaserver for the map servers list),
  /// periodically invokes onProgress callback and finally invokes onDownloaded
  /// callback. Both callbacks will be invoked on the main thread.
  void DownloadMapFile(QueuedCountry && queuedCountry);

  // Removes item from m_pendingRequests queue when list of servers is not received.
  // Parent method must be called into override method.
  virtual void Remove(CountryId const & id);

  // Clears m_pendingRequests queue when list of servers is not received.
  // Parent method must be called into override method.
  virtual void Clear();

  // Returns m_pendingRequests queue when list of servers is not received.
  // Parent method must be called into override method.
  virtual QueueInterface & GetQueue();
  // Same as above, but not overridable.
  Queue & GetPendingRequests();

  // Starts downloads from m_pendingRequests queue.
  // Supposed to be called after CheckUpdates and meta check had been processed.
  void StartPendingMapDownloads();

  /**
   * @brief Async file download as string buffer from meta-server (for small files only).
   * Callback will be skipped on download error.
   * @param[in]  url        Final url part like "maps/maps.json".
   * @param[in]  forceReset True - force reset current request, if any.
   */
  void DownloadAsStringFromMeta(std::string url, std::function<bool(std::string const &)> && callback, bool forceReset = false);
  /**
   * @brief Async file download as string buffer from a server suggested by meta-server (for small files only).
   * Fetches server list from the metaserver for the current data version first.
   * Callback will be skipped on download error.
   * @param[in]  url        Final url part like "maps/2026.04.14/260420/countries.txt".
   * @param[in]  forceReset True - force reset current request, if any.
   */
  void DownloadAsString(std::string url, std::function<bool(std::string const &)> && callback, bool forceReset = false);

  // Used in tests only.
  void SetServersList(ServersList const & serversList);

  void SetDownloadingPolicy(DownloadingPolicy * policy);
  void SetDataVersion(int64_t version) { m_dataVersion = version; }

  /// Reset after changes, e.g. map download URL.
  void ResetMetaConfig();

  /// @name Legacy functions for Android resources downloading routine (initial World download).
  /// @{
  void EnsureMetaConfigReady(std::function<void()> && callback);
  std::vector<std::string> MakeUrlListLegacy(std::string const & fileName) const;
  /// @}

protected:
  bool IsDownloadingAllowed() const;
  // Produces download urls for all servers.
  std::vector<std::string> MakeUrlList(std::string const & relativeUrl) const;

  // Synchronously loads a list of map servers from the metaserver.
  // On dl failure fallbacks to the hardcoded list.
  MetaConfig LoadMetaConfig();

  int64_t m_dataVersion = 0;

private:
  /**
   * @brief This method is blocking and should be called on network thread.
   * Default implementation receives a list of all servers that can be asked
   * for a map file and invokes callback on the main thread (@see MetaConfigCallback as SafeCallback).
   */
  virtual void GetMetaConfig(MetaConfigCallback const & callback);
  /// Asynchronously downloads the file and saves result to provided directory.
  virtual void Download(QueuedCountry && queuedCountry) = 0;

  /// @param[in]  callback  Called in main thread (@see GetMetaConfig).
  void RunMetaConfigAsync(std::function<void()> && callback);

  /// Current file downloading request for DownloadAsString.
  using RequestT = downloader::HttpRequest;
  std::unique_ptr<RequestT> m_fileRequest;

  ServersList m_serversList;

  /// Used as guard for m_serversList assign.
  std::atomic_bool m_isMetaConfigRequested = false;

  DownloadingPolicy * m_downloadingPolicy = nullptr;

  // This queue accumulates download requests before
  // the servers list is received on the network thread.
  Queue m_pendingRequests;
};
}  // namespace storage
