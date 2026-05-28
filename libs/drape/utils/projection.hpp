#pragma once

#include "drape/drape_global.hpp"

#include "indexer/drawing_rule_def.hpp"

#include <array>

namespace dp
{
// Depth range for the orthographic projection. Values are symmetric so that the
// (kMaxDepth + kMinDepth) / depth term in MakeProjection cancels to zero, producing
// a depth offset of 0 for OpenGL/Vulkan (0.5 for Metal after the API remap).
// The static_asserts below confirm that style-rule depth values and overlay priorities
// fit within this range. An earlier version of this code used asymmetric bounds, which
// caused the depth-bias term to shift the clip planes and silently cull near-plane
// geometry — that is the bug referenced in the original TODO comment; it is fixed.
float constexpr kMinDepth = -25000.0f;
float constexpr kMaxDepth = 25000.0f;

static_assert(kMinDepth <= drule::kMinLayeredDepthBg && drule::kMaxLayeredDepthFg <= kMaxDepth);
static_assert(kMinDepth <= -drule::kOverlaysMaxPriority && drule::kOverlaysMaxPriority <= kMaxDepth);

std::array<float, 16> MakeProjection(dp::ApiVersion apiVersion, float left, float right, float bottom, float top);
}  // namespace dp
