#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>

#include <exception>
#include <string>

namespace build_style
{
QString RunBuildingPhonePack(QString const & stylesDir, QString const & targetDir)
{
  using std::to_string, std::runtime_error;

  if (!QDir(stylesDir).exists())
    throw runtime_error("Styles directory does not exist " + stylesDir.toStdString());

  if (!QDir(targetDir).exists())
    throw runtime_error("target directory does not exist" + targetDir.toStdString());

#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  // Prefer the repo-local venv's python3 (provisioned by tools/unix/activate_venv.sh),
  // since it has the protobuf version this script needs and the GUI app's environment
  // won't have that venv activated the way a shell running configure.sh would.
  QString const venvPython =
      JoinPathQt({QString(GetPlatform().ResourcesDir().c_str()), "..", ".venv", "bin", "python3"});
  QString const pythonInterpreter = QFile::exists(venvPython) ? venvPython : QString("python3");
  return ExecProcess(pythonInterpreter,
                     {GetExternalPath("generate_styles_override.py", "", "../tools/python"), stylesDir, targetDir});
#else
  return ExecProcess("python",
                     {GetExternalPath("generate_styles_override.py", "", "../tools/python"), stylesDir, targetDir});
#endif
}
}  // namespace build_style
