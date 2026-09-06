#pragma once

#include "base/string_utils.hpp"

#include "drape/accessibility_node_info.hpp"
#include "drape/overlay_handle.hpp"

#include "geometry/screenbase.hpp"

namespace dp
{
// Note this holds a copy of dp::AccessibilityNodeInfo in case the OverlayHandle gets deallocated
// after we have already returned this from the FrontendRenderer.
class AccessibilityNodeContext
{
public:
  AccessibilityNodeContext(OverlayID const & overlayID, uint8_t overlaySubID, m2::RectD const & bounds,
                           OverlayHandle::Rects const & shape, AccessibilityNodeInfo const & accessibilityInfo);
  AccessibilityNodeContext(ref_ptr<OverlayHandle> overlayHandle, ScreenBase const & screen);

  [[nodiscard]] OverlayID const & GetOverlayID() const;
  [[nodiscard]] uint8_t GetOverlaySubID() const;
  [[nodiscard]] m2::RectD const & GetBounds() const;
  [[nodiscard]] OverlayHandle::Rects const & GetShape() const;
  [[nodiscard]] AccessibilityNodeInfo const & GetNodeInfo() const;

  // TODO layer/z-index info, to allow the presenter to get the top one
  // TODO non-rectangular bounds

  friend std::string DebugPrint(AccessibilityNodeContext const & context)
  {
    return DebugPrint(context.m_overlayID) + "-" + strings::to_string(context.m_overlaySubID) + ":" +
           DebugPrint(context.m_nodeInfo) + "@" + DebugPrint(context.m_bounds);
  }

private:
  OverlayID m_overlayID;
  uint8_t m_overlaySubID;
  m2::RectD m_bounds;
  OverlayHandle::Rects m_shape;
  AccessibilityNodeInfo m_nodeInfo;
};

}  // namespace dp
