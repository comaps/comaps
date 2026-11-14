#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace interop
{
using CountryIds = std::vector<std::string>;

struct Progress
{
  int64_t m_downloaded = 0;
  int64_t m_total = 0;
};

CountryIds OutdatedCountries(CountryIds const & ids);
std::string CountryName(std::string const & id);
uint64_t CountrySize(std::string const & id);
Progress OverallProgress(CountryIds const & ids);
bool HasFailedCountries(CountryIds const & ids);
bool IsAutoRetryDownloadFailed();
}  // namespace interop
