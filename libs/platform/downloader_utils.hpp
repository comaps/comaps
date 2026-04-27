#pragma once

#include <cstdint>
#include <string>

namespace downloader
{
// Build an url to download a map file from.
std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion = 0);

// Used in libs/platform/background_downloader_ios.mm
std::string GetFilePathByUrl(std::string const & url);
}  // namespace downloader
