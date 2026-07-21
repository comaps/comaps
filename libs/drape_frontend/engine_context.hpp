#pragma once

#include "drape_frontend/custom_features_context.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include <functional>
#include <vector>

namespace dp
{
class TextureManager;
}  // namespace dp

namespace df
{
class Message;
class MetalineManager;

class EngineContext
{
public:
  EngineContext(TileKey tileKey, ref_ptr<ThreadsCommutator> commutator, ref_ptr<dp::TextureManager> texMng,
                ref_ptr<MetalineManager> metalineMng, CustomFeaturesContextWeakPtr customFeaturesContext,
                bool is3dBuildingsEnabled, bool isTrafficEnabled, bool isolinesEnabled, double indoorLevel,
                std::vector<double> indoorLevels, std::vector<m2::RectD> indoorPolygonRects,
                int8_t mapLangIndex);

  TileKey const & GetTileKey() const { return m_tileKey; }
  bool Is3dBuildingsEnabled() const { return m_3dBuildingsEnabled; }
  bool IsTrafficEnabled() const { return m_trafficEnabled; }
  bool IsolinesEnabled() const { return m_isolinesEnabled; }
  double GetIndoorLevel() const { return m_indoorLevel; }
  std::vector<double> const & GetIndoorLevels() const { return m_indoorLevels; }
  std::vector<m2::RectD> const & GetIndoorPolygonRects() const { return m_indoorPolygonRects; }
  int8_t GetMapLangIndex() const { return m_mapLangIndex; }
  CustomFeaturesContextWeakPtr GetCustomFeaturesContext() const { return m_customFeaturesContext; }
  ref_ptr<dp::TextureManager> GetTextureManager() const;
  ref_ptr<MetalineManager> GetMetalineManager() const;

  void BeginReadTile();
  void Flush(TMapShapes && shapes);
  void FlushOverlays(TMapShapes && shapes);
  void FlushTrafficGeometry(TrafficSegmentsGeometry && geometry);
  void EndReadTile();

private:
  void PostMessage(drape_ptr<Message> && message);

  TileKey m_tileKey;
  ref_ptr<ThreadsCommutator> m_commutator;
  ref_ptr<dp::TextureManager> m_texMng;
  ref_ptr<MetalineManager> m_metalineMng;
  CustomFeaturesContextWeakPtr m_customFeaturesContext;
  bool m_3dBuildingsEnabled;
  bool m_trafficEnabled;
  bool m_isolinesEnabled;
  double m_indoorLevel;
  std::vector<double> m_indoorLevels;
  std::vector<m2::RectD> m_indoorPolygonRects;
  int8_t m_mapLangIndex;
};
}  // namespace df
