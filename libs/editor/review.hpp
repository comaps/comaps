#pragma once

#include "base/exception.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include "indexer/map_object.hpp"

#include <optional>
#include <string>

namespace reviews
{

DECLARE_EXCEPTION(ReviewEditorException, RootException);

enum class ReviewEditorApp
{
  Mangrove,
  MapComplete
};

std::optional<std::string> GetReviewEditorApp(osm::MapObject const & mapObject);
std::optional<std::string> GetReviewEditorUrl(osm::MapObject const & mapObject);

// exposed for testing
namespace internal
{

/// Calculate the uncertainty figure for use in geo URI
uint Uncertainty(osm::MapObject const & mapObject);

struct Mangrove
{
  [[nodiscard]] static ReviewEditorApp EditorApp() { return ReviewEditorApp::Mangrove; }
  [[nodiscard]] static std::optional<std::string> EditorUrl(osm::MapObject const & mapObject);
};

struct MapCompleteTheme
{
  std::string const mThemeId;

  [[nodiscard]] static ReviewEditorApp EditorApp() { return ReviewEditorApp::MapComplete; }
  [[nodiscard]] std::optional<std::string> EditorUrl(osm::MapObject const & mapObject) const;
};

}  // namespace internal

}  // namespace reviews
