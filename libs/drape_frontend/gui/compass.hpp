#pragma once

#include "drape_frontend/gui/shape.hpp"

#include "drape/pointers.hpp"

namespace dp
{
class GraphicsContext;
class TextureManager;
}  // namespace dp

namespace gui
{
class Compass : public Shape
{
public:
  explicit Compass(gui::Position const & position) : Shape(position) {}

  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex,
                                TTapHandler const & tapHandler) const;
};
}  // namespace gui
