#pragma once

#include <atomic>

namespace df
{
// Runtime toggle for the "non-downloaded areas" tint. Used to tell the geometry builder
// (which runs on the backend/read thread) whether the mwm-border fill should be rendered
// invisibly so it serves only as a stencil mask for the tint pass (which runs on the
// frontend thread). It is written only when the user toggles the feature, so a plain
// atomic is sufficient.
extern std::atomic<bool> g_nonDownloadedMaskEnabled;
}  // namespace df