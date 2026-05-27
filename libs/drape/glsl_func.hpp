#pragma once

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL  // Required for glm/gtx/rotate_vector.hpp (GTX = experimental extensions).
                                 // GLM >= 1.0 promotes GTX rotate_vector to stable; remove this define
                                 // and update the include to <glm/ext/...> when the bundled glm is upgraded.
#include "std/glm_gtx_rotate_vector.hpp"

namespace glsl
{
using glm::cross;
using glm::distance;
using glm::dot;
using glm::length;
using glm::normalize;

using glm::rotate;
using glm::scale;
using glm::translate;
using glm::transpose;
}  // namespace glsl
