#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/socket.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "coding/file_writer.hpp"

#include "std/windows.hpp"

#include <functional>

#include <direct.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sys/stat.h>
#include <sys/types.h>

static bool GetUserWritableDir(std::string & outDir)
{
  char pathBuf[MAX_PATH] = {0};
  if (SUCCEEDED(::SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pathBuf)))
  {
    outDir = pathBuf;
    ::CreateDirectoryA(outDir.c_str(), NULL);
    outDir += "\\CoMaps\\";
    ::CreateDirectoryA(outDir.c_str(), NULL);
    return true;
  }
  return false;
}

/// @return Full path to the executable file
static bool GetPathToBinary(std::string & outPath)
{
  // get path to executable
  char pathBuf[MAX_PATH] = {0};
  if (0 < ::GetModuleFileNameA(NULL, pathBuf, MAX_PATH))
  {
    outPath = pathBuf;
    return true;
  }
  return false;
}

namespace platform
{
std::unique_ptr<Socket> CreateSocket()
{
  return std::unique_ptr<Socket>();
}
}  // namespace platform

namespace
{
bool IsResourcesDir(std::string const & dir)
{
  return Platform::IsFileExistsByFullPath(dir + "\\welcome.html");
}
}  // namespace

Platform::Platform()
{
  std::string exePath;
  CHECK(GetPathToBinary(exePath), ("Can't get path to binary"));
  exePath.erase(exePath.find_last_of('\\'));
  // exePath is now the directory containing the executable.

  // Resources directory search order:
  // 1. MWM_RESOURCES_DIR environment variable override
  // 2. data\ next to executable (dev build junction or release install)
  // 3. CoMaps.app\Contents\Resources\ (cmake bundle next to executable)
  // 4. ..\share\comaps\data\ (FHS-style install)
  // 5. Up to 3 parent directories (development build only)
  if (char const * envDir = ::getenv("MWM_RESOURCES_DIR"))
    m_resourcesDir = std::string(envDir) + "\\";

  if (m_resourcesDir.empty() && IsResourcesDir(exePath + "\\data"))
    m_resourcesDir = exePath + "\\data\\";

  if (m_resourcesDir.empty() && IsResourcesDir(exePath + "\\CoMaps.app\\Contents\\Resources"))
    m_resourcesDir = exePath + "\\CoMaps.app\\Contents\\Resources\\";

  if (m_resourcesDir.empty() && IsResourcesDir(exePath + "\\..\\share\\comaps\\data"))
    m_resourcesDir = exePath + "\\..\\share\\comaps\\data\\";

#ifndef RELEASE
  if (m_resourcesDir.empty())
  {
    std::string devPath = exePath;
    for (int i = 0; i < 3 && m_resourcesDir.empty(); ++i)
    {
      auto const slash = devPath.find_last_of('\\');
      if (slash == std::string::npos)
        break;
      devPath.erase(slash);
      if (IsResourcesDir(devPath + "\\data"))
        m_resourcesDir = devPath + "\\data\\";
    }
  }
#endif

  // Writable directory:
  // 1. MWM_WRITABLE_DIR environment variable override (dev builds)
  // 2. %LOCALAPPDATA%\CoMaps\ (always, for installed builds)
  //
  // We intentionally do NOT use a write-test on the resources directory.
  // A write-test would incorrectly pass when:
  //  - The app runs elevated (e.g. launched from an admin installer), making Program Files writable.
  //  - The exe is installed per-user in %LOCALAPPDATA%, where the data dir is always writable,
  //    causing maps to be stored next to the exe and deleted on uninstall.
  // Dev builds that need a local writable dir should set MWM_WRITABLE_DIR.
  if (char const * envDir = ::getenv("MWM_WRITABLE_DIR"))
    m_writableDir = std::string(envDir) + "\\";
  else
    CHECK(GetUserWritableDir(m_writableDir), ("Can't get writable directory"));

  m_settingsDir = m_writableDir;
  char pathBuf[MAX_PATH] = {0};
  GetTempPathA(MAX_PATH, pathBuf);
  m_tmpDir = pathBuf;

  m_guiThread = std::make_unique<platform::GuiThread>();

  LOG(LINFO, ("Resources Directory:", m_resourcesDir));
  LOG(LINFO, ("Writable Directory:", m_writableDir));
  LOG(LINFO, ("Tmp Directory:", m_tmpDir));
  LOG(LINFO, ("Settings Directory:", m_settingsDir));
}

bool Platform::IsFileExistsByFullPath(std::string const & filePath)
{
  return ::GetFileAttributesA(filePath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// static
void Platform::DisableBackupForFile(std::string const & filePath) {}

// static
std::string Platform::GetCurrentWorkingDirectory() noexcept
{
  char path[MAX_PATH];
  char const * const dir = getcwd(path, MAX_PATH);
  if (dir == nullptr)
    return {};
  return dir;
}

// static
Platform::EError Platform::RmDir(std::string const & dirName)
{
  if (_rmdir(dirName.c_str()) != 0)
    return ErrnoToError();
  return ERR_OK;
}

// static
Platform::EError Platform::GetFileType(std::string const & path, EFileType & type)
{
  struct _stat32 stats;
  if (_stat32(path.c_str(), &stats) != 0)
    return ErrnoToError();
  if (stats.st_mode & _S_IFREG)
    type = EFileType::Regular;
  else if (stats.st_mode & _S_IFDIR)
    type = EFileType::Directory;
  else
    type = EFileType::Unknown;
  return ERR_OK;
}

std::string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

std::string Platform::DeviceModel() const
{
  return {};
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  // @TODO Add implementation
  return EConnectionType::CONNECTION_NONE;
}

Platform::ChargingStatus Platform::GetChargingStatus()
{
  return Platform::ChargingStatus::Plugged;
}

uint8_t Platform::GetBatteryLevel()
{
  // This value is always 100 for desktop.
  return 100;
}

Platform::TStorageStatus Platform::GetWritableStorageStatus(uint64_t neededSize) const
{
  ULARGE_INTEGER freeSpace;
  if (0 == ::GetDiskFreeSpaceExA(m_writableDir.c_str(), &freeSpace, NULL, NULL))
  {
    LOG(LWARNING, ("GetDiskFreeSpaceEx failed with error", GetLastError()));
    return STORAGE_DISCONNECTED;
  }

  if (freeSpace.u.LowPart + (static_cast<uint64_t>(freeSpace.u.HighPart) << 32) < neededSize)
    return NOT_ENOUGH_SPACE;

  return STORAGE_OK;
}

bool Platform::IsDirectoryEmpty(std::string const & directory)
{
  return PathIsDirectoryEmptyA(directory.c_str());
}

bool Platform::GetFileSizeByFullPath(std::string const & filePath, uint64_t & size)
{
  HANDLE hFile =
      CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    SCOPE_GUARD(autoClose, std::bind(&CloseHandle, hFile));
    LARGE_INTEGER fileSize;
    if (0 != GetFileSizeEx(hFile, &fileSize))
    {
      size = fileSize.QuadPart;
      return true;
    }
  }
  return false;
}

namespace
{
enum class FileTimeType
{
  Creation,
  Modification
};
time_t GetFileTime(std::string const & path, FileTimeType fileTimeType)
{
  HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    LOG(LERROR, ("GetFileTime CreateFileA failed for", path, "with error", strerror(errno)));
    return 0;  // TODO(AB): Refactor to return std::optional<time_t>.
  }

  SCOPE_GUARD(autoClose, std::bind(&CloseHandle, hFile));

  FILETIME ft;
  FILETIME * ftCreate = nullptr;
  FILETIME * ftLastWrite = nullptr;

  switch (fileTimeType)
  {
  case FileTimeType::Creation: ftCreate = &ft; break;
  case FileTimeType::Modification: ftLastWrite = &ft; break;
  }

  if (!::GetFileTime(hFile, ftCreate, nullptr, ftLastWrite))
  {
    LOG(LERROR, ("GetFileTime ::GetFileTime failed for", path, "with error", strerror(errno)));
    return 0;  // TODO(AB): Refactor to return std::optional<time_t>.
  }

  ULARGE_INTEGER ull;
  ull.LowPart = ft.dwLowDateTime;
  ull.HighPart = ft.dwHighDateTime;
  return static_cast<time_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);
}
}  // namespace

// static
time_t Platform::GetFileCreationTime(std::string const & path)
{
  // TODO(AB): Refactor to return std::optional<time_t>.
  return GetFileTime(path, FileTimeType::Creation);
}

// static
time_t Platform::GetFileModificationTime(std::string const & path)
{
  // TODO(AB): Refactor to return std::optional<time_t>.
  return GetFileTime(path, FileTimeType::Modification);
}

void Platform::GetSystemFontNames(FilesList & res) const {}
