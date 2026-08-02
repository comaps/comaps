#pragma once

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

/**
 * Do not subclass
 * See docs/SCREEN_READERS.md for more info
 */
struct AccessibilityNodeInfo
{
  AccessibilityNodeInfo(std::string accessibilityLabel, ExplorationType explorationType)
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
  /* Do not add any pointers or refs here, as this struct gets copied between threads */
};
}  // namespace dp
