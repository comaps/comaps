#pragma once

#include "base/stable_id_table.hpp"
#include "drape/accessibility_data.hpp"
#include "drape/accessibility_node_context.hpp"
#include "geometry/point2d.hpp"

namespace dp
{
// only safe to use from the platform ui thread (Gui)
class AccessibilityPresenter
{
public:
  using TUpdateCallback =
      std::function<void(std::vector<TAccessibilityStableID> removed, std::vector<TAccessibilityStableID> updated,
                         std::vector<TAccessibilityStableID> added)>;

  [[nodiscard]] TAccessibilityStableID GetNodeAtPoint(m2::PointD) const;
  void GetAllNodes(TAccessibilityStableIDContainer &) const;
  [[nodiscard]] std::optional<ref_ptr<AccessibilityNodeContext>> GetNode(TAccessibilityStableID) const;
  /*
  void GetTextureInfoAtPoint(m2::PointD); // TODO

  void GetHomingInfoForPoint(m2::PointD); // TODO
  void SetHomingMode(); // TODO

  void SetPresentationMode(); // TODO eg read icons or just vibrate - currently that's hardcoded, should move to
  categories or sth
  */
  void Update(drape_ptr<AccessibilityData> && newData);

  /**
   * Set the callback, which is called immediately after every frame that modified the accessibility tree.
   * @param cb A callback that receives the (no longer valid) ID of removed overlays and the (still valid) ID of updated
   * or new overlays. The removed overlays' IDs may be immediately reused for new overlays, but this does not imply any
   * correlation.
   */
  void SetUpdateCallback(std::optional<TUpdateCallback> const & cb) { m_updateCallback = cb; }

private:
  static std::pair<OverlayID, uint8_t> const s_tombstone;

  std::optional<drape_ptr<AccessibilityData>> m_data;
  base::StableIDTable<std::pair<OverlayID, uint8_t>, s_tombstone, TAccessibilityStableID, 1> m_stableIDTable;
  // The values are dangling unless the old m_data is still kept alive.
  // If you can guarantee the old m_data is alive, you can cast this safely.
  std::map<std::pair<OverlayID, uint8_t>, ref_ptr<void>> m_lastOverlays;
  std::map<std::pair<OverlayID, uint8_t>, ref_ptr<AccessibilityNodeContext>> m_overlays;

  std::optional<TUpdateCallback> m_updateCallback;
};

}  // namespace dp
