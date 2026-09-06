#pragma once

#include "drape/accessibility_node_context.hpp"
#include "drape/accessibility_traits.hpp"
#include "geometry/tree4d.hpp"

#define DRAPE_TYPICAL_A11Y_NODE_COUNT 50

namespace dp
{
using TAccessibilityStableID = int;

using TAccessibilityNodeContextContainer =
    buffer_vector<ref_ptr<AccessibilityNodeContext>, DRAPE_TYPICAL_A11Y_NODE_COUNT>;
using TAccessibilityStableIDContainer = buffer_vector<TAccessibilityStableID, DRAPE_TYPICAL_A11Y_NODE_COUNT>;
// Note that the Accessibility Tree is a spatial 4d tree, not a semantic tree like we usually have in a11y.
using AccessibilityTree = m4::Tree<ref_ptr<AccessibilityNodeContext>, AccessibilityTraits>;

class AccessibilityData
{
public:
  AccessibilityData();

  explicit AccessibilityData(
      AccessibilityTree && tree,
      buffer_vector<drape_ptr<AccessibilityNodeContext>, DRAPE_TYPICAL_A11Y_NODE_COUNT> && nodeContexts)
    : m_tree(std::move(tree))
    , m_nodeContexts(std::move(nodeContexts))
  {}
  // TODO texture regions etc may come later

  [[nodiscard]] AccessibilityTree const & GetTree() const { return m_tree; }

  void Add(ref_ptr<OverlayHandle> overlays, ScreenBase const & screen);

private:
  AccessibilityTree m_tree;
  buffer_vector<drape_ptr<AccessibilityNodeContext>, DRAPE_TYPICAL_A11Y_NODE_COUNT> m_nodeContexts;
};
}  // namespace dp
