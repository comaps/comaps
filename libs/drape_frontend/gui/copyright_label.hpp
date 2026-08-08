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
class CopyrightLabel : public Shape
{
  using TBase = Shape;

public:
  explicit CopyrightLabel(Position const & position);
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const;
};
}  // namespace gui
