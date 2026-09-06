#include "drape/accessibility_data.hpp"
#include "drape/accessibility_node_context.hpp"

namespace dp
{
AccessibilityData::AccessibilityData() = default;

void AccessibilityData::Add(ref_ptr<OverlayHandle> overlay, ScreenBase const & screen)
{
  if (!overlay->IsVisible())
    return;
  // TODO store the order of addition into the context, so we can get z-order correct in the presenter
  auto context = make_unique_dp<AccessibilityNodeContext>(overlay, screen);
  if (context->GetNodeInfo().m_explorationType == NOT_EXPLORABLE)
    return;
  // technically this ref dangles but since we keep the drape_ptr in nodeContexts it is safe.
  m_tree.Add(make_ref(context));
  m_nodeContexts.push_back(std::move(context));
}
}  // namespace dp
