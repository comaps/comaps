#include "drape/accessibility_node_context.hpp"

#include <utility>

namespace dp
{
// FIXME: the FeatureID actually contains a shared_ptr to MwmInfo so can we safely share it? I *think* it's safe.
AccessibilityNodeContext::AccessibilityNodeContext(OverlayID const & overlayID, uint8_t overlaySubID,
                                                   m2::RectD const & bounds, OverlayHandle::Rects const & shape,
                                                   AccessibilityNodeInfo const & accessibilityInfo)
  : m_overlayID(overlayID)
  , m_overlaySubID(overlaySubID)
  , m_bounds(bounds)
  , m_shape(shape)
  , m_nodeInfo(accessibilityInfo)
{}

AccessibilityNodeContext::AccessibilityNodeContext(ref_ptr<OverlayHandle> overlayHandle, ScreenBase const & screen)
  : AccessibilityNodeContext(overlayHandle->GetOverlayID(), overlayHandle->GetOverlaySubID(),
                             overlayHandle->GetPixelRect(screen, screen.isPerspective()), OverlayHandle::Rects{},
                             overlayHandle->GetAccessibilityInfo())
{
  overlayHandle->GetPixelShape(screen, screen.isPerspective(), m_shape);
}

OverlayID const & AccessibilityNodeContext::GetOverlayID() const
{
  return m_overlayID;
}
uint8_t AccessibilityNodeContext::GetOverlaySubID() const
{
  return m_overlaySubID;
}
m2::RectD const & AccessibilityNodeContext::GetBounds() const
{
  return m_bounds;
}
OverlayHandle::Rects const & AccessibilityNodeContext::GetShape() const
{
  return m_shape;
}
AccessibilityNodeInfo const & AccessibilityNodeContext::GetNodeInfo() const
{
  return m_nodeInfo;
}
}  // namespace dp
