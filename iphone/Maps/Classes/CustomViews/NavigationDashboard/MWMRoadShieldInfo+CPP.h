#pragma once

@class MWMRoadShieldInfo;

#ifdef __cplusplus
#include "routing/following_info.hpp"

MWMRoadShieldInfo * MWMBuildRoadShieldInfo(routing::FollowingInfo::RoadShieldInfo const & info);
#endif
