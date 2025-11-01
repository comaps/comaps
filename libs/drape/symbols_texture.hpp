#pragma once

#include "drape/texture.hpp"

#include <unordered_map>
#include <string>
#include <vector>

namespace dp
{
class SymbolsTexture : public Texture
{
public:
  class SymbolKey : public Key
  {
  public:
    explicit SymbolKey(std::string const & symbolName);
    ResourceType GetType() const override;
    std::string const & GetSymbolName() const;

  private:
    std::string m_symbolName;
  };

  class SymbolInfo : public ResourceInfo
  {
  public:
    explicit SymbolInfo(m2::RectF const & texRect);
    ResourceType GetType() const override;
  };

  SymbolsTexture(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                 std::string const & textureName, ref_ptr<HWTextureAllocator> allocator);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;

  void Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                  ref_ptr<HWTextureAllocator> allocator);
  void Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                  ref_ptr<HWTextureAllocator> allocator, std::vector<drape_ptr<HWTexture>> & internalTextures);

  inline bool IsSymbolContained(std::string const & symbolName) const
  {
    return m_definition.find(symbolName) != m_definition.end();
  }

private:
  void Fail(ref_ptr<dp::GraphicsContext> context);
  void Load(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
            ref_ptr<HWTextureAllocator> allocator);

  std::string m_name;
  mutable std::unordered_map<std::string, SymbolInfo> m_definition;
};
}  // namespace dp
