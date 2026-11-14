#include "storage_interop.hpp"

#include <CoreApi/Framework.h>

#include <algorithm>
#include <iterator>

namespace interop
{
CountryIds OutdatedCountries(CountryIds const & ids)
{
  auto const & storage = GetFramework().GetStorage();
  CountryIds result;
  std::copy_if(ids.begin(), ids.end(), std::back_inserter(result),
               [&storage](std::string const & id) { return !storage.HasLatestVersion(id); });
  return result;
}

std::string CountryName(std::string const & id)
{
  storage::NodeAttrs attrs;
  GetFramework().GetStorage().GetNodeAttrs(id, attrs);
  return attrs.m_nodeLocalName;
}

uint64_t CountrySize(std::string const & id)
{
  storage::NodeAttrs attrs;
  GetFramework().GetStorage().GetNodeAttrs(id, attrs);
  return attrs.m_mwmSize;
}

Progress OverallProgress(CountryIds const & ids)
{
  auto const progress = GetFramework().GetStorage().GetOverallProgress(ids);
  return {progress.m_bytesDownloaded, progress.m_bytesTotal};
}

bool HasFailedCountries(CountryIds const & ids)
{
  return GetFramework().GetStorage().CheckFailedCountries(ids);
}

bool IsAutoRetryDownloadFailed()
{
  return GetFramework().GetDownloadingPolicy().IsAutoRetryDownloadFailed();
}
}  // namespace interop
