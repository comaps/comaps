#pragma once

#include "drape/pointers.hpp"

#include <limits>
#include <vector>

class ScreenBase;

namespace dp
{
class DebugRenderer;
class GraphicsContext;
class OverlayHandle;
class OverlayTree;
class VertexArrayBuffer;

// RenderBucket is the unit of flushed geometry passed from BackendRenderer to FrontendRenderer.
//
// It pairs a VertexArrayBuffer (the raw vertex/index geometry for one RenderState) with an
// optional list of OverlayHandles (interactive hit-test elements such as POI labels or icons).
//
// Lifecycle:
//   BackendRenderer: Batcher::EndSession() finalises a bucket and calls the TFlushFn callback,
//     which posts a FlushTileMessage carrying the bucket to FrontendRenderer.
//   FrontendRenderer: the bucket is stored inside a RenderGroup (keyed by TileKey + RenderState).
//     Each frame, FrontendRenderer calls CollectOverlayHandles() to register visible overlays with
//     the OverlayTree, then Render() to draw the geometry.
//
// Thread safety: a bucket is created on BackendRenderer's thread, ownership is transferred via
// drape_ptr move to FrontendRenderer; after that point it is only touched on the FR thread.
class RenderBucket
{
public:
  explicit RenderBucket(drape_ptr<VertexArrayBuffer> && buffer);
  ~RenderBucket();

  ref_ptr<VertexArrayBuffer> GetBuffer();
  drape_ptr<VertexArrayBuffer> && MoveBuffer();

  size_t GetOverlayHandlesCount() const;
  drape_ptr<OverlayHandle> PopOverlayHandle();
  ref_ptr<OverlayHandle> GetOverlayHandle(size_t index);
  void AddOverlayHandle(drape_ptr<OverlayHandle> && handle);

  void Update(ScreenBase const & modelView);
  void CollectOverlayHandles(ref_ptr<OverlayTree> tree);
  bool HasOverlayHandles() const;
  //! \return true if tree completely invalidated and next call has no sense
  bool RemoveOverlayHandles(ref_ptr<OverlayTree> tree);
  void SetOverlayVisibility(bool isVisible);
  void Render(ref_ptr<GraphicsContext> context, bool drawAsLine);

  // Only for testing! Don't use this function in production code!
  void RenderDebug(ref_ptr<GraphicsContext> context, ScreenBase const & screen,
                   ref_ptr<DebugRenderer> debugRectRenderer) const;

  // Only for testing! Don't use this function in production code!
  template <typename ToDo>
  void ForEachOverlay(ToDo const & todo)
  {
    for (drape_ptr<OverlayHandle> const & h : m_overlay)
      todo(make_ref(h));
  }

  void SetFeatureMinZoom(int minZoom);
  int GetMinZoom() const { return m_featuresMinZoom; }

private:
  void BeforeUpdate();

  int m_featuresMinZoom = std::numeric_limits<int>::max();

  std::vector<drape_ptr<OverlayHandle>> m_overlay;
  drape_ptr<VertexArrayBuffer> m_buffer;
};
}  // namespace dp
