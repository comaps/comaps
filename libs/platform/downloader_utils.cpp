#include "platform/downloader_utils.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/url.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "std/target_os.hpp"

#include "defines.hpp"
#include "private.h"

namespace
{
/// @todo(pastk): review diffs dirs structure and move inside maps folder.
std::string const kDiffsPath = "diffs";
}  // namespace

namespace downloader
{

std::string GetFileDownloadUrl(std::string const & fileName, int64_t dataVersion, uint64_t diffVersion /* = 0 */)
{
  if (diffVersion == 0)
    return url::Join(MAPS_BASE_URL, MAP_SERIES, strings::to_string(dataVersion), url::UrlEncode(fileName));

  return url::Join(kDiffsPath, MAP_SERIES, strings::to_string(dataVersion), strings::to_string(diffVersion),
                   url::UrlEncode(fileName));
}

std::string GetFilePathByUrl(std::string const & url)
{
  auto const urlComponents = strings::Tokenize(url, "/");
  CHECK_GREATER(urlComponents.size(), 3, (urlComponents));
  CHECK_LESS(urlComponents.size(), 6, (urlComponents));

  uint64_t dataVersion = 0;
  CHECK(strings::to_uint(urlComponents[2], dataVersion), ());

  std::string mwmFile = url::UrlDecode(urlComponents.back());
  // remove extension
  mwmFile = mwmFile.substr(0, mwmFile.find('.'));

  auto const fileType = urlComponents[0] == kDiffsPath ? MapFileType::Diff : MapFileType::Map;
  return platform::GetFileDownloadPath(dataVersion, mwmFile, fileType);
}

}  // namespace downloader
