#pragma once

#include <QtCore/QString>

namespace build_style
{
void BuildDrawingRules(QString const & mapcssFile, QString const & outputDir, QString const & prioDir);
void ApplyDrawingRules(QString const & outputDir);
}  // namespace build_style
