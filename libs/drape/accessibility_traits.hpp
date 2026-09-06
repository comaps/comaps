#pragma once
#include "drape/accessibility_node_context.hpp"
#include "drape/pointers.hpp"
#include "geometry/screenbase.hpp"

namespace dp
{
class AccessibilityTraits
{
public:
  [[nodiscard]] m2::RectD LimitRect(ref_ptr<AccessibilityNodeContext> const & info) const { return info->GetBounds(); }
  [[nodiscard]] ScreenBase const & GetModelView() const { return m_modelView; }

private:
  ScreenBase m_modelView;
};
}  // namespace dp
