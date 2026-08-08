#pragma once

#include "storage/downloader_queue_universal.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/storage_defines.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace storage
{
/// This class encapsulates HTTP requests for receiving server lists
/// and file downloading.
//
// *NOTE*, this class is not thread-safe.
class HttpMapFilesDownloader : public MapFilesDownloader
{
public:
  virtual ~HttpMapFilesDownloader();

  // MapFilesDownloader overrides:
  void Remove(CountryId const & id) override;
  void Clear() override;
  QueueInterface & GetQueue() override;

private:
  // MapFilesDownloader overrides:
  void Download(QueuedCountry && queuedCountry) override;

  void Download();

  void OnMapFileDownloaded(QueuedCountry const & queuedCountry, downloader::HttpRequest & request);
  void OnMapFileDownloadingProgress(QueuedCountry const & queuedCountry, downloader::HttpRequest & request);

  std::unique_ptr<downloader::HttpRequest> m_request;
  Queue m_queue;

  DECLARE_THREAD_CHECKER(m_checker);
};
}  // namespace storage
