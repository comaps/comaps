#include "storage/map_files_downloader.hpp"

#include "storage/downloading_policy.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/http_client.hpp"
#include "platform/locale.hpp"
#include "platform/platform.hpp"
#include "platform/servers_list.hpp"
#include "platform/settings.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace storage
{
void MapFilesDownloader::DownloadMapFile(QueuedCountry && queuedCountry)
{
  m_pendingRequests.Append(std::move(queuedCountry));
}

void MapFilesDownloader::RunMetaConfigAsync(std::function<void()> && callback)
{
  m_isMetaConfigRequested = true;

  GetPlatform().RunTask(Platform::Thread::Network, [this, callback = std::move(callback)]()
  {
    GetMetaConfig([this, callback = std::move(callback)](MetaConfig const & metaConfig)
    {
      m_serversList = metaConfig.m_serversList;
      settings::Update(metaConfig.m_settings);
      callback();

      // Reset flag to invoke servers list downloading next time if current request has failed.
      m_isMetaConfigRequested = false;
    });
  });
}

void MapFilesDownloader::Remove(CountryId const & id)
{
  if (!m_pendingRequests.IsEmpty())
    m_pendingRequests.Remove(id);
}

void MapFilesDownloader::Clear()
{
  m_pendingRequests.Clear();
}

QueueInterface & MapFilesDownloader::GetQueue()
{
  return m_pendingRequests;
}

Queue & MapFilesDownloader::GetPendingRequests()
{
  return m_pendingRequests;
}

void MapFilesDownloader::StartPendingMapDownloads()
{
  if (!m_pendingRequests.IsEmpty())
    EnsureMetaConfigReady([this]()
    {
      LOG(LINFO, ("Starting pending map downloads..."));
      m_pendingRequests.ForEachCountry([this](QueuedCountry & country) { Download(std::move(country)); });
      m_pendingRequests.Clear();
    });
}

void MapFilesDownloader::DownloadAsStringFromMeta(std::string url, std::function<bool(std::string const &)> && callback,
                                          bool forceReset /* = false */)
{
  if (m_fileRequest && !forceReset)
    return;

  Platform & pl = GetPlatform();
  std::string metaServerUrl = pl.MetaServerUrl();
  // If user sets a custom download server, skip metaserver entirely.
  std::string const customServer = pl.CustomMapServerUrl();
  if (!customServer.empty())
  {
    LOG(LINFO, ("Using custom map server URL:", customServer));

    metaServerUrl = customServer;
  }

  m_fileRequest.reset(RequestT::Get(url::Join(metaServerUrl, url),
                                    [this, callback = std::move(callback)](RequestT & request)
  {
    bool deleteRequest = true;
    auto const & buffer = request.GetData();

    LOG(LDEBUG, ("DownloadAsStringFromMeta: status=", request.GetStatus(), "bytes=", buffer.size()));

    // Update deleteRequest flag if new download was requested in callback.
    deleteRequest = !callback(buffer);

    if (deleteRequest)
      m_fileRequest.reset();
  }));
}

void MapFilesDownloader::DownloadAsString(std::string url, std::function<bool(std::string const &)> && callback,
                                          bool forceReset /* = false */)
{
  EnsureMetaConfigReady([this, forceReset, url = std::move(url), callback = std::move(callback)]()
  {
    if ((m_fileRequest && !forceReset) || m_serversList.empty())
      return;

    /// @todo(pastk): try to download from several servers - similar to map files
    // Servers are sorted from best to worst.
    m_fileRequest.reset(RequestT::Get(url::Join(m_serversList.front(), url),
                                      [this, callback = std::move(callback)](RequestT & request)
    {
      bool deleteRequest = true;
      auto const & buffer = request.GetData();

      LOG(LDEBUG, ("DownloadAsString: status=", request.GetStatus(), "bytes=", buffer.size()));

      // Update deleteRequest flag if new download was requested in callback.
      deleteRequest = !callback(buffer);

      if (deleteRequest)
        m_fileRequest.reset();
    }));
  });
}

void MapFilesDownloader::EnsureMetaConfigReady(std::function<void()> && callback)
{
  /// @todo Implement logic if m_metaConfig is "outdated".
  if (!m_serversList.empty())
  {
    callback();
  }
  else if (!m_isMetaConfigRequested)
  {
    RunMetaConfigAsync(std::move(callback));
  }
  else
  {
    // skip this request without callback call
  }
}

std::vector<std::string> MapFilesDownloader::MakeUrlListLegacy(std::string const & fileName) const
{
  return MakeUrlList(downloader::GetFileDownloadUrl(fileName, m_dataVersion));
}

void MapFilesDownloader::SetServersList(ServersList const & serversList)
{
  m_serversList = serversList;
}

void MapFilesDownloader::SetDownloadingPolicy(DownloadingPolicy * policy)
{
  m_downloadingPolicy = policy;
}

bool MapFilesDownloader::IsDownloadingAllowed() const
{
  return m_downloadingPolicy == nullptr || m_downloadingPolicy->IsDownloadingAllowed();
}

std::vector<std::string> MapFilesDownloader::MakeUrlList(std::string const & relativeUrl) const
{
  std::vector<std::string> urls;
  urls.reserve(m_serversList.size());
  for (auto const & server : m_serversList)
    urls.emplace_back(url::Join(server, relativeUrl));

  return urls;
}

std::string GetAcceptLanguage()
{
  auto const locale = platform::GetCurrentLocale();
  return locale.m_language + "-" + locale.m_country;
}

// static
MetaConfig MapFilesDownloader::LoadMetaConfig()
{
  Platform & pl = GetPlatform();

  // If user sets a custom download server, skip metaserver entirely.
  std::string const customServer = pl.CustomMapServerUrl();
  if (!customServer.empty())
  {
    LOG(LINFO, ("Using custom map server URL:", customServer));

    MetaConfig metaConfig;
    metaConfig.m_serversList = {customServer};
    return metaConfig;
  }

  std::string const metaServerUrl = url::Join(pl.MetaServerUrl(), "servers");
  std::string httpResult;

  if (!metaServerUrl.empty())
  {
    LOG(LINFO, ("Requesting metaserver", metaServerUrl));
    platform::HttpClient request(metaServerUrl);
    request.SetRawHeader("X-OM-DataVersion", std::to_string(m_dataVersion));
    request.SetRawHeader("X-OM-AppVersion", pl.Version());
    request.SetRawHeader("Accept-Language", GetAcceptLanguage());
    request.SetTimeout(10.0);  // timeout in seconds
    request.RunHttpRequest(httpResult);
  }

  auto metaConfig = downloader::ParseMetaConfig(httpResult);
  if (!metaConfig)
  {
    metaConfig = downloader::ParseMetaConfig(pl.DefaultUrlsJSON());
    CHECK(metaConfig, ());
    LOG(LWARNING, ("Can't get metaserver configuration, using default servers:", metaConfig->m_serversList));
  }
  else
  {
    LOG(LINFO, ("Got servers list:", metaConfig->m_serversList));
  }

  CHECK(!metaConfig->m_serversList.empty(), ());
  return *metaConfig;
}

void MapFilesDownloader::GetMetaConfig(MetaConfigCallback const & callback)
{
  callback(LoadMetaConfig());
}

void MapFilesDownloader::ResetMetaConfig()
{
  m_serversList.clear();
  m_isMetaConfigRequested = false;
}

}  // namespace storage
