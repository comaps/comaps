# Drape Rendering Engine

Drape is CoMaps' GPU rendering engine. It converts map features (roads, buildings, POIs, labels) into pixels on screen using OpenGL ES3, Vulkan, or Metal depending on platform.

## High-Level Architecture

Drape is split into two libraries:

- **`libs/drape/`** — GPU primitives: textures, shaders, vertex buffers, batching, render state
- **`libs/drape_frontend/`** — Map-level logic: tile management, feature-to-geometry conversion, thread orchestration

The central coordinator is `DrapeEngine`, which owns two renderer threads and a message bus:

```
          Main/UI Thread
               │
          DrapeEngine
          ┌────┴────┐
          │         │
  FrontendRenderer  BackendRenderer
  (RenderThread)    (ResourceUploadThread)
          │         │
          └────┬────┘
        ThreadsCommutator
```

## Threading Model

### RenderThread — FrontendRenderer

Owns the GPU context and the screen framebuffer. Responsibilities:
- Processes user input events (pan, zoom, rotate) via `UserEventStream`
- Runs animations (position, zoom, route following) via `MyPositionController`
- Maintains the overlay tree (labels, pins) for hit-testing and visibility culling
- Renders all `RenderGroup`s each frame in depth-layer order

### ResourceUploadThread — BackendRenderer

Prepares geometry for the GPU without touching the framebuffer. Responsibilities:
- Reads map features from MWM files via `ReadManager` → `ReadMWMTask`
- Converts features to drawable shapes via `RuleDrawer` → `ApplyFeatureFunctors`
- Batches geometry into `RenderBucket`s via `BatchersPool` → `Batcher`
- Uploads textures and vertex data; sends finished buckets to FrontendRenderer

### Communication

All cross-thread calls go through `ThreadsCommutator` as typed `Message` objects (100+ subtypes in `message_subclasses.hpp`). Neither renderer calls the other directly. Each renderer drains its message queue at the start of each frame before doing any work.

## Render Pipeline — From Raw Data to Screen

```
Map data on disk (MWM files)
    │
    │  ReadManager queues a ReadMWMTask per visible TileKey
    ▼
RuleDrawer::Draw()
    │  Applies style rules to each feature
    │  Creates MapShape subclasses (AreaShape, LineShape, TextShape, PoiSymbolShape…)
    ▼
Batcher::Insert*()
    │  Accumulates vertex/index geometry per RenderState (shader + blend mode + textures)
    │  When a bucket is full, flushes it via callback
    ▼
RenderBucket  (VertexArrayBuffer + optional OverlayHandles)
    │  Sent via message to FrontendRenderer
    ▼
FrontendRenderer::RenderGroups
    │  Organized by TileKey + RenderState
    │  Grouped into DepthLayers (2D base → 3D buildings → overlays)
    ▼
RenderScene() each frame
    │  For each layer, for each group, for each bucket:
    │    - Bind shader program
    │    - Upload pending vertex data
    │    - Draw call
    ▼
Screen framebuffer
```

## Key Abstractions

### TileKey
A `(x, y, zoomLevel)` coordinate identifying one map tile, plus a `generation` counter. The generation increments whenever the tile's geometry needs to be regenerated (e.g. after a style change). FrontendRenderer uses generations to discard stale buckets; BackendRenderer uses them to skip redundant reads.

### Batcher
The geometry accumulator used in BackendRenderer. A `BatchersPool` allocates one `Batcher` per tile. Code calls `Batcher::InsertTriangleList()` / `InsertLineStrip()` etc. to add primitives. When a `Batcher` session ends (`EndSession()`), it fires a flush callback for each `RenderState` that received geometry, delivering a `RenderBucket` to FrontendRenderer.

### RenderState
An immutable descriptor combining: GPU program (shader pair), blending mode, depth test flags, and texture bindings. `Batcher` uses it as a key — geometry with the same state ends up in the same bucket, minimising GPU state changes during rendering.

### RenderBucket
A `VertexArrayBuffer` (one VAO/VBO/IBO) plus an array of `OverlayHandle`s for interactive elements. Owned exclusively by a `RenderGroup`. Its CPU-side data is populated in BackendRenderer; `Build()` creates the actual GPU objects on first use in RenderThread.

### RenderGroup
Groups all `RenderBucket`s that share a `TileKey` and `RenderState`. Belongs to exactly one `DepthLayer`. FrontendRenderer iterates groups in layer order each frame.

### MapShape
Abstract base for all drawable geometry. Subclasses (`AreaShape`, `LineShape`, `TextShape`, `PoiSymbolShape`, `CircleShape`…) implement `Draw(ref_ptr<Batcher>, ref_ptr<TextureManager>)`. They translate style parameters and feature geometry into `Batcher::Insert*()` calls, requesting texture regions from `TextureManager` as needed.

### TextureManager
The GPU resource allocator for all 2D textures. Manages atlas packing for four resource types:
- **Symbols** — PNG icons for POIs and UI elements
- **Glyphs** — Rendered font glyphs, packed into glyph atlases on demand
- **StipplePen** — 1D textures encoding dashed-line patterns
- **Colors** — 1D texture encoding solid colours (avoids per-vertex colour attributes)

`TextureManager` is shared between threads; regions are allocated from BackendRenderer, but GPU texture objects are created and bound from RenderThread.

### OverlayHandle / OverlayTree
`OverlayHandle` wraps an interactive element (label, pin, selection marker) with screen bounds, priority, and a unique `OverlayID`. `OverlayTree` is a spatial index rebuilt each frame in FrontendRenderer to resolve visibility (higher-priority overlays displace lower-priority ones that overlap) and to perform hit-testing for taps.

### ThreadsCommutator
The message bus. Code calls `PostMessage(ThreadName, make_unique_dp<SomeMessage>(…))`. Registered renderers implement `AcceptMessage(ref_ptr<Message>)` and dispatch on `Message::Type`. This keeps cross-thread coupling to a single dispatch point.

## GPU Abstraction Layer

`libs/drape/` abstracts three backends:

| Class | OpenGL ES3 | Vulkan | Metal |
|---|---|---|---|
| `HWTexture` | `gl/gl_hw_texture` | `vulkan/vulkan_texture` | `metal/metal_texture` |
| `GpuProgram` | `gl/gl_gpu_program` | `vulkan/vulkan_gpu_program` | `metal/metal_gpu_program` |
| `GraphicsContext` | `gl/gl_context` | `vulkan/vulkan_base_context` | `metal/metal_context` |

Callers use the base-class interface exclusively; platform selection happens at `DrapeEngine` creation time via `GraphicsContextFactory`.

## Depth Layers

FrontendRenderer renders geometry in this order (back to front):

1. **2D geometry** — filled areas, road casings, road fills, low-zoom labels
2. **3D geometry** — extruded buildings
3. **Overlay layer** — POI icons, road shields, town/street labels
4. **Route layer** — navigation route and turn arrows
5. **Traffic layer** — traffic colour overlays
6. **Transit layer** — public-transit scheme

Each layer is a `RenderLayer` holding a list of `RenderGroup`s sorted by depth value within the layer.

## Frame Lifecycle

```
FrontendRenderer::Routine (runs every frame):
  1. Drain message queue — accept new buckets, discard stale tiles, apply style changes
  2. ProcessEvents — consume touch/gesture events, run animations one step
  3. PrepareScene — compute visible tile set for current viewport
  4. UpdateScene — request missing tiles from BackendRenderer, discard invisible ones
  5. BuildOverlayTree — re-evaluate label/pin visibility for this frame
  6. RenderScene — issue GPU draw calls for all layers
  7. Present framebuffer
```

If nothing changed (no messages, no animations, no pending tiles), the renderer skips drawing after `kMaxInactiveFrames` idle frames to save battery.

## Known Limitations and Open Work

The codebase contains ~200 `TODO`/`FIXME`/`@todo` comments. Notable clusters:

- **Text rendering** (`glyph_manager.cpp`, `harfbuzz_shaping.cpp`, `text_layout.cpp`) — Glyph caching, HarfBuzz cluster handling, and bidirectional text have multiple open questions around performance and correctness at non-standard font sizes.
- **Overlay placement** (`overlay_tree.hpp/.cpp`) — Parent-finding is O(n) per overlay; a comment suggests caching by `OverlayID` would speed this up.
- **Line rendering** (`line_shape.cpp`) — Known artefacts at joins/caps at certain zoom levels; filtration logic is approximate.
- **Vulkan** (`vulkan_base_context.cpp:197`) — `FIXME: infinite timeout` on Android device wakeup.
- **DrapeEngine** (`drape_engine.hpp:239`) — Custom feature rendering API (`DrapeApi`) is suspected unused in production; the comment notes possible ad-POI origin.
- **render_state_extension** (`render_state_extension.hpp:30`, `render_group.hpp:34`) — Polymorphic extension point acknowledged as a design smell; proposed replacement is a plain struct.

## Directory Map

```
libs/drape/
  batcher.hpp/.cpp          — Geometry accumulator; flushes RenderBuckets
  gpu_program.hpp/.cpp      — Shader program abstraction
  render_bucket.hpp/.cpp    — VAO + overlay handles for one draw call
  render_state.hpp/.cpp     — Immutable GPU state descriptor (key for batching)
  texture.hpp/.cpp          — Base texture + atlas region types
  texture_manager.hpp/.cpp  — Allocates symbols, glyphs, stipple, colour regions
  vertex_array_buffer.hpp   — CPU→GPU vertex/index upload and draw
  overlay_handle.hpp/.cpp   — Screen-space bounds + hit-test for interactive elements
  overlay_tree.hpp/.cpp     — Spatial index for overlay visibility and hit-testing
  drape_global.hpp          — Shared enums and constants (Anchor, DepthLayer, …)
  gl/, vulkan/, metal/      — Per-backend implementations

libs/drape_frontend/
  drape_engine.hpp/.cpp     — Public API; owns both renderers and commutator
  frontend_renderer.hpp/.cpp — RenderThread: scene management, GPU drawing
  backend_renderer.hpp/.cpp  — ResourceUploadThread: data reading, geometry generation
  threads_commutator.hpp/.cpp — Message bus between threads
  message.hpp               — Message base class and Type enum
  message_subclasses.hpp    — All 100+ concrete message types
  read_manager.hpp/.cpp     — Schedules and cancels tile read tasks
  rule_drawer.hpp/.cpp      — Applies style rules, creates MapShapes
  apply_feature_functors.hpp/.cpp — Per-feature-type geometry generators
  batcher_pool.hpp          — Manages one Batcher per TileKey
  render_group.hpp/.cpp     — Groups buckets by TileKey + RenderState
  tile_key.hpp              — Tile coordinate + generation counter
  tile_utils.hpp            — Tile coverage, neighbour queries, zoom clamping
  map_data_provider.hpp     — Callback interface to MWM feature data
  my_position_controller.hpp/.cpp — GPS/location animation and rendering
  overlay_manager.hpp/.cpp  — Wraps OverlayTree for FrontendRenderer
  route_renderer.hpp/.cpp   — Navigation route drawing
  traffic_renderer.hpp/.cpp — Traffic colour overlay drawing
```
