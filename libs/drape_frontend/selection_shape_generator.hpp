#pragma once

#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

namespace dp
{
class GraphicsContext;
class TextureManager;
}  // namespace dp

namespace df
{
class MapDataProvider;
class MetalineManager;
class RenderNode;

class SelectionShapeGenerator
{
public:
  static drape_ptr<RenderNode> GenerateSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                       ref_ptr<dp::TextureManager> mng);

  static drape_ptr<RenderNode> GenerateTrackSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                            ref_ptr<dp::TextureManager> mng);

  static drape_ptr<RenderNode> GenerateSelectionGeometry(ref_ptr<dp::GraphicsContext> context,
                                                         FeatureID const & feature, ref_ptr<dp::TextureManager> mng,
                                                         ref_ptr<MetalineManager> metalineMng,
                                                         MapDataProvider & mapDataProvider);
};
}  // namespace df
