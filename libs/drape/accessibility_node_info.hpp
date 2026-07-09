#pragma once

/*
 * Plumbing data for screen readers
 * Data flow diagram:
 * df/rule_drawer.cpp -> df/apply_feature_functors.cpp
 *                                       |
 *                                       v
 * e.g. dp/line_overlay.cpp <- e.g. dp/line_shape.cpp
 *             |                         |
 *             v                         v
 *   dp/accessibility_node_info.hpp -> dp/overlay_handle.hpp
 *                                       |
 *                                       v
 *   dp/accessibility_data.hpp <- df/frontend_renderer.cpp
 *            \                          |
 *                                       v
 *             -----------------> df/drape_engine.cpp
 *                                       |
 *                                       v
 *    platform impl  <->  dp/accessibility_presenter.cpp
 */

#include <string>

#include "base/string_utils.hpp"

namespace dp
{
enum ExplorationType
{
  // Ignore for accessibility.
  NOT_EXPLORABLE = 0,
  // Only read the name when we are confident the user specifically wants to explore this location
  ANNOUNCE_LABEL_ON_LONG_HOVER = 1,
  // Always read the label when the user goes nearby
  ANNOUNCE_LABEL_ALWAYS = 2,
  // Always tell the user there is something here e.g. with a chime or vibration
  SIGNAL_PRESENCE_ALWAYS = 4,
  // Play tone or vibration signal to help the user home to the location of this overlay
  SIGNAL_HOMING = 8,
};

constexpr ExplorationType operator|(ExplorationType a, ExplorationType b)
{
  return static_cast<ExplorationType>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr ExplorationType ExplorationType_MASK_FOCUSABLE =
    ANNOUNCE_LABEL_ON_LONG_HOVER | ANNOUNCE_LABEL_ALWAYS | SIGNAL_PRESENCE_ALWAYS;

struct AccessibilityNodeInfo
{
  AccessibilityNodeInfo(std::string accessibilityLabel, ExplorationType explorationType)
    // Note: already copied in the constructor invocation, so we can move it now
    : m_accessibilityLabel(std::move(accessibilityLabel))
    , m_explorationType(explorationType)
  {}

  AccessibilityNodeInfo() : AccessibilityNodeInfo("", NOT_EXPLORABLE) {}

  friend std::string DebugPrint(AccessibilityNodeInfo const & info)
  {
    return "(\"" + info.m_accessibilityLabel + "\":" + strings::to_string(info.m_explorationType);
  }

  std::string m_accessibilityLabel;
  ExplorationType m_explorationType;
};
}  // namespace dp
