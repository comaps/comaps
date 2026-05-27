#pragma once

#include "drape_frontend/message_acceptor.hpp"
#include "drape_frontend/threads_commutator.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/graphics_context_factory.hpp"
#include "drape/texture_manager.hpp"

#include "base/thread.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

namespace df
{
using OnGraphicsContextInitialized = std::function<void()>;

// BaseRenderer is the common base for FrontendRenderer (RenderThread) and BackendRenderer
// (ResourceUploadThread). Each subclass owns a dedicated OS thread and a graphics context.
//
// Thread lifecycle:
//   StartThread() spawns the thread, which calls CreateContext() → OnContextCreate() and then
//   enters the render loop (IterateRenderLoop()). Each loop iteration drains the message queue
//   (MessageAcceptor::ProcessMessages()) and calls RenderFrame() for FrontendRenderer.
//   StopThread() posts a stop message and joins the thread.
//
// Rendering can be suspended and resumed without destroying the thread:
//   SetRenderingDisabled(destroySurface=true)  — surface lost (e.g. app backgrounded on Android)
//   SetRenderingDisabled(destroySurface=false) — pause without surface loss
//   SetRenderingEnabled()  — surface recreated; re-creates the context if it was destroyed.
//   Subclasses are notified via OnRenderingEnabled() / OnRenderingDisabled().
//
// CanReceiveMessages() returns false when the renderer is stopped and messages should not be
// posted. IterateRenderLoop() is called by the platform's GL thread (Qt) instead of the
// internal thread on platforms that require rendering on the main/platform thread.
class BaseRenderer : public MessageAcceptor
{
public:
  struct Params
  {
    Params(dp::ApiVersion apiVersion, ref_ptr<ThreadsCommutator> commutator,
           ref_ptr<dp::GraphicsContextFactory> factory, ref_ptr<dp::TextureManager> texMng,
           OnGraphicsContextInitialized const & onGraphicsContextInitialized)
      : m_apiVersion(apiVersion)
      , m_commutator(commutator)
      , m_oglContextFactory(factory)
      , m_texMng(texMng)
      , m_onGraphicsContextInitialized(onGraphicsContextInitialized)
    {}

    dp::ApiVersion m_apiVersion;
    ref_ptr<ThreadsCommutator> m_commutator;
    ref_ptr<dp::GraphicsContextFactory> m_oglContextFactory;
    ref_ptr<dp::TextureManager> m_texMng;
    OnGraphicsContextInitialized m_onGraphicsContextInitialized;
  };

  BaseRenderer(ThreadsCommutator::ThreadName name, Params const & params);

  bool CanReceiveMessages();

  void IterateRenderLoop();

  void SetRenderingEnabled(ref_ptr<dp::GraphicsContextFactory> contextFactory);
  void SetRenderingDisabled(bool const destroySurface);

  bool IsRenderingEnabled() const;

  dp::ApiVersion GetApiVersion() const { return m_apiVersion; }

protected:
  dp::ApiVersion m_apiVersion;
  ref_ptr<ThreadsCommutator> m_commutator;
  ref_ptr<dp::GraphicsContextFactory> m_contextFactory;
  ref_ptr<dp::GraphicsContext> m_context;
  ref_ptr<dp::TextureManager> m_texMng;

  void StartThread();
  void StopThread();

  void CreateContext();

  void CheckRenderingEnabled();

  virtual std::unique_ptr<threads::IRoutine> CreateRoutine() = 0;

  virtual void RenderFrame() = 0;

  virtual void OnContextCreate() = 0;
  virtual void OnContextDestroy() = 0;

  virtual void OnRenderingEnabled() {}
  virtual void OnRenderingDisabled() {}

private:
  using TCompletionHandler = std::function<void()>;

  void IterateRenderLoopImpl();

  threads::Thread m_selfThread;
  ThreadsCommutator::ThreadName m_threadName;

  std::mutex m_renderingEnablingMutex;
  std::condition_variable m_renderingEnablingCondition;
  std::atomic<bool> m_isEnabled;
  TCompletionHandler m_renderingEnablingCompletionHandler;
  std::mutex m_completionHandlerMutex;
  bool m_wasNotified;
  std::atomic<bool> m_wasContextReset;

  OnGraphicsContextInitialized m_onGraphicsContextInitialized;
  static std::atomic<uint8_t> m_contextCounter;

  bool FilterContextDependentMessage(ref_ptr<Message> msg);
  void SetRenderingEnabled(bool const isEnabled);
  void Notify();
  void WakeUp();
};
}  // namespace df
