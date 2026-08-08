#pragma once

#include "drape/pointers.hpp"

#include <string>
#include <vector>

class ScreenBase;

namespace dp
{
class GraphicsContext;
}  // namespace dp

namespace gpu
{
class ProgramManager;
}  // namespace gpu

namespace df
{
struct DrapeApiRenderProperty;
struct FrameValues;

class DrapeApiRenderer
{
public:
  DrapeApiRenderer() = default;

  void AddRenderProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                           std::vector<drape_ptr<DrapeApiRenderProperty>> && properties);
  void RemoveRenderProperty(std::string const & id);
  void Clear();

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              FrameValues const & frameValues);

private:
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_properties;
};
}  // namespace df
