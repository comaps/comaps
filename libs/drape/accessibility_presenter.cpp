#include "drape/accessibility_presenter.hpp"

namespace dp
{
std::pair<OverlayID, uint8_t> const AccessibilityPresenter::s_tombstone = std::make_pair(OverlayID{}, 0);

[[nodiscard]]
TAccessibilityStableID AccessibilityPresenter::GetNodeAtPoint(m2::PointD point) const
{
  TAccessibilityNodeContextContainer result;

  // empty rect will select only overlapping items
  auto rect = m2::RectD(point, point);

  if (!m_data)
    return 0;

  (*m_data)->GetTree().ForEachInRect(rect, [&result, point](ref_ptr<AccessibilityNodeContext> const & c)
  {
    if (!(c->GetNodeInfo().m_explorationType & ExplorationType_MASK_FOCUSABLE))
      return;
    for (m2::RectF shape : c->GetShape())
    {
      if (m2::RectD(shape).IsPointInside(point))
      {
        result.push_back(c);
        break;
      }
    }
  });

  if (result.empty())
    return 0;

  // TODO make sure this the last-drawn i.e. top. I think that will require manual sorting, by bucket-layer and priority
  // maybe.
  return m_stableIDTable.GetStableID(std::make_pair(result[0]->GetOverlayID(), result[0]->GetOverlaySubID()));
}

void AccessibilityPresenter::GetAllNodes(TAccessibilityStableIDContainer & out) const
{
  ASSERT(out.empty(), ());
  if (!m_data)
    return;

  auto const tree = (*m_data)->GetTree();
  out.reserve(tree.GetSize());
  tree.ForEach([&out, this](ref_ptr<AccessibilityNodeContext> const & context)
  { out.push_back(m_stableIDTable.GetStableID(std::make_pair(context->GetOverlayID(), context->GetOverlaySubID()))); });
  // TODO sort by spiral shape from centre
}

std::optional<ref_ptr<AccessibilityNodeContext>> AccessibilityPresenter::GetNode(TAccessibilityStableID const id) const
{
  try
  {
    return m_overlays.at(m_stableIDTable.At(id));
  }
  catch (std::out_of_range const &)
  {
    return {};
  }
}

void AccessibilityPresenter::Update(drape_ptr<AccessibilityData> && newData)
{
  auto const tree = newData->GetTree();

  auto & lastOverlays =
      reinterpret_cast<std::map<std::pair<OverlayID, uint8_t>, ref_ptr<AccessibilityNodeContext>> &>(m_lastOverlays);
  std::swap(m_overlays, lastOverlays);
  m_overlays.clear();
  std::vector<TAccessibilityStableID> removed, updated, added;
  tree.ForEach([this, &updated, &added, &lastOverlays](ref_ptr<AccessibilityNodeContext> const & context)
  {
    auto const overlayID = std::make_pair(context->GetOverlayID(), context->GetOverlaySubID());

    auto const id = m_stableIDTable.EnsureStableID(overlayID);
    ASSERT(!m_overlays.contains(overlayID), (DebugPrint(*m_overlays[overlayID].get()), DebugPrint(*context.get())));
    m_overlays.insert_or_assign(overlayID, context);

    if (!lastOverlays.contains(overlayID))
      added.push_back(id);
    else if (context != lastOverlays.at(overlayID))
      updated.push_back(id);
  });

  std::list<std::pair<OverlayID, uint8_t>> toRemove;
  auto lastOverlayIDs = std::views::keys(m_lastOverlays);
  auto overlayIDs = std::views::keys(m_overlays);
  std::ranges::set_difference(lastOverlayIDs, overlayIDs, std::back_insert_iterator(toRemove));
  for (auto const & overlayID : toRemove)
    removed.push_back(m_stableIDTable.Remove(overlayID));

  // these must be the last lines, or we get dangling pointers to the old m_data
  m_data = std::move(newData);
  if (m_updateCallback)
    m_updateCallback->operator()(removed, updated, added);
}
}  // namespace dp
