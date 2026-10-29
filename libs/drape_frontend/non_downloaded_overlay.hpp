#pragma once

#include <atomic>

namespace df
{
// Runtime toggle for "non-downloaded areas" tint. Used for geometry builder and tint pass, written only on user toggle
extern std::atomic<bool> g_nonDownloadedMaskEnabled;
}  // namespace df