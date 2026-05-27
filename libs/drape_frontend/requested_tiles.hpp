#pragma once

#include "drape_frontend/tile_utils.hpp"

#include "geometry/screenbase.hpp"

#include <mutex>

namespace df
{
// RequestedTiles is a thread-safe single-slot mailbox between FrontendRenderer and BackendRenderer.
//
// FrontendRenderer calls Set() each time the viewport changes (new tile set computed).
// BackendRenderer calls GetTiles() / GetParams() to consume the latest request before launching
// reads. Only the most recent request is kept — if the viewport changes again before the backend
// has consumed the previous request, the old request is silently replaced. This is intentional:
// there is no point reading tiles for a viewport the user has already scrolled away from.
//
// CheckTileKey() can be called from either thread to verify that a given tile is still in the
// most recently requested set (used to cancel in-flight reads for tiles that are no longer needed).
class RequestedTiles
{
public:
  RequestedTiles() = default;
  void Set(ScreenBase const & screen, bool have3dBuildings, bool forceRequest, bool forceUserMarksRequest,
           TTilesCollection && tiles);
  TTilesCollection GetTiles();
  void GetParams(ScreenBase & screen, bool & have3dBuildings, bool & forceRequest, bool & forceUserMarksRequest);
  bool CheckTileKey(TileKey const & tileKey) const;

private:
  TTilesCollection m_tiles;
  ScreenBase m_screen;
  bool m_have3dBuildings = false;
  bool m_forceRequest = false;
  bool m_forceUserMarksRequest = false;
  mutable std::mutex m_mutex;
};
}  // namespace df
