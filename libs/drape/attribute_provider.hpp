#pragma once

#include "drape/binding_info.hpp"
#include "drape/pointers.hpp"

#include <vector>

namespace dp
{
// AttributeProvider is a read cursor over caller-owned vertex data, consumed by Batcher::Insert*().
//
// A provider has N streams, each mapping to one BindingInfo (a set of vertex attributes, e.g.
// position, UV, colour). Streams allow vertex attributes to be stored in separate arrays
// (stream-of-structures or structure-of-arrays) rather than forcing a single interleaved layout.
//
// Usage:
//   1. Construct with streamCount = number of attribute arrays, vertexCount = vertices to upload.
//   2. Call InitStream() once per stream, pointing each to a caller-managed data buffer.
//   3. Pass the provider to Batcher::Insert*(), which calls GetRawPointer() / Advance() to pull
//      vertices in chunks that fit the current VertexArrayBuffer capacity.
//   4. The provider does NOT own the data — the caller must keep the buffers alive until the
//      Batcher call returns.
class AttributeProvider
{
public:
  AttributeProvider(uint8_t streamCount, uint32_t vertexCount);

  bool IsDataExists() const;
  uint32_t GetVertexCount() const;

  uint8_t GetStreamCount() const;
  void const * GetRawPointer(uint8_t streamIndex);
  BindingInfo const & GetBindingInfo(uint8_t streamIndex) const;

  void Advance(uint32_t vertexCount);

  void InitStream(uint8_t streamIndex, BindingInfo const & bindingInfo, ref_ptr<void> data);

  void Reset(uint32_t vertexCount);
  void UpdateStream(uint8_t streamIndex, ref_ptr<void> data);

private:
  uint32_t m_vertexCount;

  struct AttributeStream
  {
    BindingInfo m_binding;
    ref_ptr<void> m_data;
  };
  std::vector<AttributeStream> m_streams;
#ifdef DEBUG
  void CheckStreams() const;
  void InitCheckStream(uint8_t streamIndex);
  std::vector<bool> m_checkInfo;
#endif
};
}  // namespace dp
