#include "platform/safe_mode.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <cstdio>
#include <string>

namespace safe_mode
{
namespace
{
static constexpr char const * kSentinelFilename = "load_in_progress";
static bool s_active = false;
static std::string s_sentinelPath;
}  // namespace

bool Init()
{
  s_sentinelPath = base::JoinPath(GetPlatform().WritableDir(), kSentinelFilename);

  uint64_t size = 0;
  s_active = Platform::GetFileSizeByFullPath(s_sentinelPath, size);

  if (s_active)
  {
    // Clear immediately so a normal restart works after the user resolves the problem.
    std::remove(s_sentinelPath.c_str());
    LOG(LWARNING, ("Safe mode activated: previous run crashed during map loading."));
  }

  return s_active;
}

bool IsActive()
{
  return s_active;
}

void MarkLoadStarted()
{
  // Synchronous write — must survive a native crash (SIGSEGV).
  FILE * f = std::fopen(s_sentinelPath.c_str(), "wb");
  if (f)
  {
    std::fputc(1, f);
    std::fflush(f);
    std::fclose(f);
  }
  else
  {
    LOG(LERROR, ("Failed to write safe mode sentinel:", s_sentinelPath));
  }
}

void MarkLoadComplete()
{
  std::remove(s_sentinelPath.c_str());
}
}  // namespace safe_mode
