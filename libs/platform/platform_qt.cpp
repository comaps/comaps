#include "platform/constants.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"

#include "base/logging.hpp"

#include <memory>

#include <boost/regex.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLocale>

std::unique_ptr<ModelReader> Platform::GetReader(std::string const & file, std::string searchScope) const
{
  return std::make_unique<FileReader>(ReadPathForFile(file, std::move(searchScope)), READER_CHUNK_LOG_SIZE,
                                      READER_CHUNK_LOG_COUNT);
}

bool Platform::GetFileSizeByName(std::string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName), size);
  }
  catch (RootException const &)
  {
    return false;
  }
}

void Platform::GetFilesByRegExp(std::string const & directory, boost::regex const & regexp, FilesList & outFiles)
{
  QDir dir(QString::fromUtf8(directory.c_str()));
  int const count = dir.count();

  for (int i = 0; i < count; ++i)
  {
    std::string name = dir[i].toStdString();
    if (boost::regex_search(name.begin(), name.end(), regexp))
      outFiles.push_back(std::move(name));
  }
}

void Platform::GetAllFiles(std::string const & directory, FilesList & outFiles)
{
  QDir dir(QString::fromUtf8(directory.c_str()));

  for (int i = 0; i < dir.count(); ++i)
    outFiles.push_back(dir[i].toStdString());
}

int Platform::PreCachingDepth() const
{
  return 3;
}

int Platform::VideoMemoryLimit() const
{
  return 20 * 1024 * 1024;
}

// static
Platform::EError Platform::MkDir(std::string const & dirName)
{
  if (QDir().exists(dirName.c_str()))
    return Platform::ERR_FILE_ALREADY_EXISTS;
  if (!QDir().mkdir(dirName.c_str()))
  {
    LOG(LWARNING, ("Can't create directory: ", dirName));
    return Platform::ERR_UNKNOWN;
  }
  return Platform::ERR_OK;
}

void Platform::SetupMeasurementSystem() const
{
  auto units = measurement_utils::Units::Metric;
  if (settings::Get(settings::kMeasurementUnits, units))
    return;
  bool const isMetric = QLocale::system().measurementSystem() == QLocale::MetricSystem;
  units = isMetric ? measurement_utils::Units::Metric : measurement_utils::Units::Imperial;
  settings::Set(settings::kMeasurementUnits, units);
}

extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
