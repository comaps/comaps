#pragma once

#include "drape/pointers.hpp"

#include <map>

namespace df
{

class Message;
enum class MessagePriority;
class BaseRenderer;

// ThreadsCommutator is the message bus between Drape's two renderer threads.
//
// Neither FrontendRenderer nor BackendRenderer calls the other directly.
// All cross-thread communication goes through PostMessage(). Each BaseRenderer
// drains its queue at the start of every frame (BaseRenderer::ProcessMessages).
//
// Typical send sites:
//   - DrapeEngine posts to both threads (e.g. UpdateMapStyle → both renderers)
//   - BackendRenderer posts FlushTile to RenderThread when a tile is ready
//   - FrontendRenderer posts RequestTiles to ResourceUploadThread when tiles
//     enter the viewport
//
// Message types are defined in drape_frontend/message_subclasses.hpp.
// MessagePriority controls queue ordering — high-priority messages (e.g. user
// input) are processed before lower-priority data messages.
class ThreadsCommutator
{
public:
  enum ThreadName
  {
    RenderThread,          // FrontendRenderer — owns the GPU context and framebuffer
    ResourceUploadThread,  // BackendRenderer  — reads map data and builds geometry
  };

  void RegisterThread(ThreadName name, BaseRenderer * acceptor);
  void PostMessage(ThreadName name, drape_ptr<Message> && message, MessagePriority priority);

private:
  using TAcceptorsMap = std::map<ThreadName, BaseRenderer *>;
  TAcceptorsMap m_acceptors;
};

}  // namespace df
