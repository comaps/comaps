#pragma once

#include <cstdint>

namespace df
{
/**
 * These can only be used when all other fields of OverlayID are set to the default.
 * They represent things that are shown, but are not actually part of the map.
 */
enum OverlayIDNonMapIndex : uint32_t
{
  CirclesPackHandleRoutePreview = 1,
  CirclesPackHandleGpsTrack,
  GuiHandleScaleLabel,
  GuiHandleCopyright,
  GuiHandleCompass,
  GuiHandleRuler,
  GuiHandleRulerLabel,
  GuiHandleChoosePositionMark,
  GuiHandleWatermark,
#ifdef RENDER_DEBUG_INFO_LABELS
  GuiHandleDebugLabel = 100
#endif
  // Keep clear! GuiHandleDebugLabel is dynamically growable!
};
}  // namespace df
