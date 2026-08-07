#include "run_tests.h"

#include "platform/platform.hpp"

#include "build_common.h"

namespace build_style
{
std::pair<bool, QString> RunCurrentStyleTests()
{
  QString const program = GetExternalPath("style_tests", "style_tests.app/Contents/MacOS", "");
  QString const resourcesDir = QString::fromStdString(GetPlatform().ResourcesDir());
  QString stderrOutput;
  QString const output = ExecProcess(program, {
                                                  "--user_resource_path=" + resourcesDir,
                                                  "--data_path=" + resourcesDir,
                                              },
                                     nullptr, &stderrOutput);

  // Unfortunately test process returns 0 even if some test failed, therefore phrase
  // 'All tests passed.' is looked for to be sure that everything is OK. style_tests logs
  // everything (including that phrase) to stderr, not stdout, so both must be checked.
  QString const combined = output + stderrOutput;
  return std::make_pair(combined.contains("All tests passed."), combined);
}
}  // namespace build_style
