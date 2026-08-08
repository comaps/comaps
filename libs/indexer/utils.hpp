#pragma once

#include "indexer/mwm_set.hpp"

#include <memory>
#include <vector>

class DataSource;

namespace indexer
{
MwmSet::MwmHandle FindWorld(DataSource const & dataSource, std::vector<std::shared_ptr<MwmInfo>> const & infos);
MwmSet::MwmHandle FindWorld(DataSource const & dataSource);
}  // namespace indexer
