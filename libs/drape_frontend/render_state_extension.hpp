#pragma once

#include "shaders/programs.hpp"

#include "drape/pointers.hpp"
#include "drape/render_state.hpp"

#include <cstdint>

namespace df
{
// DepthLayer controls the coarse rendering order of map elements across a frame.
// RenderStateExtension::Less() sorts render groups by DepthLayer so all geometry in one layer
// is drawn before geometry in the next layer within a single rendering pass.
//
// The frame sequence in FrontendRenderer::RenderScene() calls dedicated render functions per
// layer in a specific order (generally back to front). Most layers follow enum order, but
// OverlayUnderBuildingLayer is an intentional exception — see below.
//
// Layer semantics (back to front, enum order):
//   GeometryLayer          — base map polygons, roads, land cover
//   Geometry3dLayer        — extruded 3D building sides and rooftops
//   UserLineLayer          — user-drawn routes and track lines (above buildings)
//   OverlayLayer           — POI icons, road shields, labels (managed by OverlayTree)
//   TransitSchemeLayer     — metro/transit overlay drawn above regular overlays
//   UserMarkLayer          — bookmarks, search pins, generic user marks
//   RoutingBottomMarkLayer — routing waypoint decorations (below route turn arrows)
//   RoutingMarkLayer       — route turn arrows and waypoint icons
//   SearchMarkLayer        — search result highlights (above routing marks)
//   GuiLayer               — HUD elements (compass, scale ruler, zoom buttons)
//   OverlayUnderBuildingLayer — POI icons for features physically inside buildings (subway
//                              entrances, parking, etc.). Sorted last in the enum but rendered
//                              BEFORE Render3dLayer in the frame sequence so that 3D buildings
//                              draw on top. Depth testing is disabled for these overlays —
//                              the under-building effect comes from explicit render order, not
//                              the depth buffer.
//
// IMPORTANT: Do not change the numeric order — it is baked into RenderStateExtension::Less().
enum class DepthLayer : uint8_t
{
  // Do not change the order.
  GeometryLayer = 0,
  Geometry3dLayer,
  UserLineLayer,
  OverlayLayer,
  TransitSchemeLayer,
  UserMarkLayer,
  RoutingBottomMarkLayer,
  RoutingMarkLayer,
  SearchMarkLayer,
  GuiLayer,
  OverlayUnderBuildingLayer,
  LayersCount
};

// RenderStateExtension carries a DepthLayer tag inside a dp::RenderState. It is polymorphic
// because dp::BaseRenderStateExtension is an abstract type used by the generic render-state
// comparison infrastructure in drape/render_state.hpp. In practice only this one subclass exists;
// the polymorphism could be replaced with a plain struct + a comparator, but doing so requires
// touching the render-state comparison throughout the drape library.
class RenderStateExtension : public dp::BaseRenderStateExtension
{
public:
  explicit RenderStateExtension(DepthLayer depthLayer);

  bool Less(ref_ptr<dp::BaseRenderStateExtension> other) const override;
  bool Equal(ref_ptr<dp::BaseRenderStateExtension> other) const override;

  DepthLayer GetDepthLayer() const { return m_depthLayer; }

private:
  DepthLayer const m_depthLayer;
};

extern DepthLayer GetDepthLayer(dp::RenderState const & state);
extern dp::RenderState CreateRenderState(gpu::Program program, DepthLayer depthLayer);

inline std::string DebugPrint(DepthLayer layer)
{
  switch (layer)
  {
  case DepthLayer::GeometryLayer: return "GeometryLayer";
  case DepthLayer::Geometry3dLayer: return "Geometry3dLayer";
  case DepthLayer::UserLineLayer: return "UserLineLayer";
  case DepthLayer::OverlayLayer: return "OverlayLayer";
  case DepthLayer::OverlayUnderBuildingLayer: return "OverlayUnderBuildingLayer";
  case DepthLayer::TransitSchemeLayer: return "TransitSchemeLayer";
  case DepthLayer::UserMarkLayer: return "UserMarkLayer";
  case DepthLayer::RoutingBottomMarkLayer: return "RoutingBottomMarkLayer";
  case DepthLayer::RoutingMarkLayer: return "RoutingMarkLayer";
  case DepthLayer::SearchMarkLayer: return "SearchMarkLayer";
  case DepthLayer::GuiLayer: return "GuiLayer";
  case DepthLayer::LayersCount: CHECK(false, ("Try to output LayersCount"));
  }
  CHECK(false, ("Unknown layer"));
  return {};
}
}  // namespace df
