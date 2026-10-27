#pragma once

@class MWMLaneInfo;

#ifdef __cplusplus
#include "routing/lanes/lane_info.hpp"

NSArray<MWMLaneInfo *> * MWMBuildLaneInfos(routing::turns::lanes::LanesInfo const & lanes);
#endif
