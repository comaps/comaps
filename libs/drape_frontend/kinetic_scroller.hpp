#pragma once

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <chrono>
#include <deque>

class ScreenBase;

namespace df
{
class Animation;

class KineticScroller
{
public:
  void Init(ScreenBase const & modelView);
  void Update(ScreenBase const & modelView);
  bool IsActive() const;
  void Cancel();
  drape_ptr<Animation> CreateKineticAnimation(ScreenBase const & modelView);

private:
  std::pair<m2::PointD, double> GetDirectionAndVelocity(ScreenBase const & modelView) const;

  using ClockT = std::chrono::steady_clock;
  static double GetDurationSeconds(ClockT::time_point const & t2, ClockT::time_point const & t1)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }

  std::deque<std::pair<m2::PointD, ClockT::time_point>> m_points;
  bool m_isActive = false;
};
}  // namespace df
