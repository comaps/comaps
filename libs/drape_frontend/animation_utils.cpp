#include "drape_frontend/animation_utils.hpp"
#include "drape_frontend/animation_constants.hpp"

namespace df
{

bool IsAnimationAllowed(double duration, ScreenBase const & screen)
{
  return duration > 0.0 && duration <= kMaxAnimationTimeSec;
}

}  // namespace df
