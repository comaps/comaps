#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>

#include <exception>
#include <string>

namespace build_style
{
QString GetStyleStatistics(QString const & mapcssMappingFile, QString const & drulesFile)
{
  if (!QFile(mapcssMappingFile).exists())
    throw std::runtime_error("mapcss-mapping file does not exist at " + mapcssMappingFile.toStdString());

  if (!QFile(drulesFile).exists())
    throw std::runtime_error("drawing-rules file does not exist at " + drulesFile.toStdString());

  // Add path to the protobuf EGG in the PROTOBUF_EGG_PATH environment variable.
  QProcessEnvironment env{QProcessEnvironment::systemEnvironment()};
  env.insert("PROTOBUF_EGG_PATH", GetProtobufEggPath());

  // Run the script.
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  // Prefer the repo-local venv's python3 (provisioned by tools/unix/activate_venv.sh),
  // since it has the protobuf version this script needs and the GUI app's environment
  // won't have that venv activated the way a shell running configure.sh would.
  QString const venvPython = JoinPathQt({GetPlatform().ResourcesDir().c_str(), "..", ".venv", "bin", "python3"});
  QString const pythonInterpreter = QFileInfo::exists(venvPython) ? venvPython : QString("python3");
  return ExecProcess(pythonInterpreter,
                     {
                         GetExternalPath("drules_info.py", "kothic/src", "../tools/python/stylesheet"),
                         mapcssMappingFile,
                         drulesFile,
                     },
                     &env);
#else
  return ExecProcess("python",
                     {
                         GetExternalPath("drules_info.py", "kothic/src", "../tools/python/stylesheet"),
                         mapcssMappingFile,
                         drulesFile,
                     },
                     &env);
#endif
}

QString GetCurrentStyleStatistics()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const mappingPath = JoinPathQt({resourceDir, "mapcss-mapping.csv"});
  QString const drulesPath = JoinPathQt({resourceDir, "drules_proto_design.bin"});
  return GetStyleStatistics(mappingPath, drulesPath);
}
}  // namespace build_style
