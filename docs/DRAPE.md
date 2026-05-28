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

---

## Open Issues and TODOs

The following is a full catalogue of `TODO`, `FIXME`, `@todo`, and `HACK` markers found across `libs/drape/` and `libs/drape_frontend/`. Items are grouped by theme. File paths are relative to the repo root.

### Text Rendering and Glyph Management

These are the densest cluster of open work, mostly owned by **AB** (Alexander Borsuk). The core problems are: HarfBuzz text-run splitting is incomplete for mixed-font and RTL strings; glyph caching is ad-hoc; and font size handling between FreeType and HarfBuzz is inconsistent.

| File | Line | Note |
|------|------|------|
| `libs/drape/glyph_manager.cpp` | 221–222 | Font size set redundantly every call; `hb_font_set_scale` should be used instead of mutating FreeType size |
| `libs/drape/glyph_manager.cpp` | 240 | Missing-glyph ID not checked |
| `libs/drape/glyph_manager.cpp` | 511 | Text runs not guaranteed to be split by font |
| `libs/drape/glyph_manager.cpp` | 563 | Invalid glyphs not fully handled |
| `libs/drape/glyph_manager.cpp` | 594 | Some substrings use different fonts than expected |
| `libs/drape/glyph_manager.cpp` | 606 | Font-to-character mapping is O(n) per character |
| `libs/drape/glyph_manager.cpp` | 612 | Font selection uses only the first character of a string — can fail for mixed-script strings |
| `libs/drape/glyph_manager.cpp` | 628 | Cache eviction strategy unclear ("Is there a better way? E.g. clear a half of the cache?") |
| `libs/drape/glyph_manager.hpp` | 16, 33 | `GlyphImage` and `Glyph` structs noted as candidates for moving to separate files |
| `libs/drape/glyph_manager.hpp` | 23 | Unclear whether metrics should store font units or floats |
| `libs/drape/glyph_manager.hpp` | 50 | `height += yOffset` line commented out — unknown if correct |
| `libs/drape/glyph.hpp` | 15 | Manual `Destroy()` call on `GlyphImage` should be replaced with RAII |
| `libs/drape/font_texture.hpp` | 51 | `Texture::ResourceInfo` should be non-abstract; `GlyphRegion` could use it directly |
| `libs/drape/font_texture.cpp` | 128 | Unclear whether a zero-size glyph is valid or an error |
| `libs/drape/font_texture.cpp` | 183 | No check that the glyph image exists before uploading to GPU |
| `libs/drape/harfbuzz_shaping.cpp` | 175 | Run-breaking does not account for Unicode block boundaries, parentheses, or control chars |
| `libs/drape/harfbuzz_shaping.cpp` | 177 | Vertical text layouts not supported |
| `libs/drape/harfbuzz_shaping.cpp` | 191 | Segment list copies data — should use indices |
| `libs/drape/harfbuzz_shaping.cpp` | 194 | Line direction taken from first segment only — wrong for mixed-direction strings |
| `libs/drape/harfbuzz_shaping.cpp` | 219, 221 | Unnecessary string conversion/allocation; runs not split by breaking chars or fonts |
| `libs/drape/harfbuzz_shaping.hpp` | 12 | `ScreenChar` uses 4 bytes where 1–2 would suffice for cache efficiency |
| `libs/drape/harfbuzz_shaping.hpp` | 30 | Segment ordering uses moves instead of index reversal |
| `libs/drape/texture_manager.hpp` | 229 | Space glyph workaround needed because `BreakIterator` is not used to split strings properly |
| `libs/drape/texture_manager.cpp` | 275 | Glyph pre-cache does not deduplicate spaces or repeated characters |
| `libs/drape/texture_manager.cpp` | 497 | Parameter name `textLanguageIndex` noted as unclear |
| `libs/drape/texture_manager.cpp` | 512 | Mutex protecting glyph lookup may be a bottleneck |
| `libs/drape_frontend/text_layout.cpp` | 88 | `yAdvance` always zero for horizontal layouts — can be simplified |
| `libs/drape_frontend/text_layout.cpp` | 279 | Conversion to `TGlyphs` possibly avoidable |
| `libs/drape_frontend/text_layout.cpp` | 292, 398 | Newlines in strings replaced with spaces as a temporary workaround |
| `libs/drape_frontend/text_layout.cpp` | 304–305 | ICU `BreakIterator` not used — word-splitting falls back to whitespace only; RTL splitting not implemented |
| `libs/drape_frontend/text_layout.cpp` | 410 | `StraightTextLayout` previously split long strings into two lines; logic removed but not replaced |
| `libs/drape_frontend/gui/gui_text.cpp` | 322 | Unclear if per-frame shaping/precaching is needed for dynamic text (e.g. speed readout) |
| `libs/drape_frontend/gui/gui_text.cpp` | 362 | No clean way to clear cached vertices from the previous frame |
| `libs/drape_frontend/gui/gui_text.cpp` | 371, 375 | Pre-calculated glyph width/height not reused |
| `libs/drape_frontend/gui/gui_text.cpp` | 409 | `yAdvance` always zero for horizontal layouts |
| `libs/drape_frontend/gui/gui_text.hpp` | 130 | Class noted for refactoring (no further detail) |

### Overlay System

| File | Line | Note |
|------|------|------|
| `libs/drape/overlay_handle.hpp` | 143 | `displayFlag` logic effectively disabled — the flag exists but is not checked |
| `libs/drape/overlay_tree.hpp` | 124 | `m_overlayIdCache` was implemented and `FindParent()` is now O(log n) — the TODO comment is stale |
| `libs/drape/overlay_tree.cpp` | 172, 220 | OverlayID is transiently invalid during country delete/re-add; assert suppressed deliberately (see inline comment) |
| `libs/drape/overlay_tree.cpp` | 481 | Point-select intentionally skips `Select(rect)` — see inline comment for why |
| `libs/drape_frontend/text_shape.cpp` | 405 | Shape classes both draw geometry and manipulate overlay priorities — should be separated |

### Feature-to-Geometry Conversion (`apply_feature_functors.cpp`)

All items below are from **pastk** (Pastuhov Konstantin) and relate to geometry quality and generator-vs-renderer responsibility.

| File | Line | Note |
|------|------|------|
| `libs/drape_frontend/apply_feature_functors.cpp` | 478 | `text-offset` style property used for anchor positioning only — offset semantics are vestigial |
| `libs/drape_frontend/apply_feature_functors.cpp` | 522 | Features like a bench under a bridge not hidden when occluded by structure |
| `libs/drape_frontend/apply_feature_functors.cpp` | 543 | Single cross-dependency with the Editor — noted as undesirable coupling |
| `libs/drape_frontend/apply_feature_functors.cpp` | 658, 666, 674, 698, 772 | Degenerate triangle filtering done at render time — should be done in the map generator |
| `libs/drape_frontend/apply_feature_functors.cpp` | 737 | Tile rect clipping uses approximate instead of exact match |
| `libs/drape_frontend/apply_feature_functors.cpp` | 858 | Area borders only work for buildings, not other polygonal areas |
| `libs/drape_frontend/apply_feature_functors.cpp` | 880, 918 | Scale/zoom computation repeated per feature instead of once per tile in `RuleDrawer` |

### Stylist and Rule Processing

| File | Line | Note |
|------|------|------|
| `libs/drape_frontend/stylist.cpp` | 55 | Secondary text forced for all road/river lines — should be opt-in via style rules |
| `libs/drape_frontend/stylist.cpp` | 96, 102 | House number `minZoom` recomputed on every feature; depends on secondary caption existence in an unclear way |
| `libs/drape_frontend/stylist.cpp` | 157 | Circle/waymarker support may be dead code (not used in current styles) |
| `libs/drape_frontend/rule_drawer.cpp` | 328 | Check cannot move to top — it suppresses only POI labels, not area geometry; see inline comment |
| `libs/drape_frontend/rule_drawer.cpp` | 461 | `MinZoom` field used for `RenderGroup` deletion optimization but the logic has been disabled for a long time |
| `libs/drape_frontend/transit_scheme_builder.cpp` | 70 | Transit casing colour is hardcoded — should be configurable in style files |

### Texture and GPU Resources

| File | Line | Note |
|------|------|------|
| `libs/drape/texture_manager.cpp` | 39 / `libs/drape/stipple_pen_resource.hpp` | `kStippleTextureWidth` (512) and `kMaxStipplePenLength` (512) should be the same constant but are defined separately |
| `libs/drape/texture_manager.cpp` | 49 | `kGlyphAreaMultiplier` — 1.2 provides headroom for tall glyphs (CJK, diacritics); 1.0 would cause drops — see inline comment |
| `libs/drape/texture_manager.cpp` | 82 | Assert suppressed non-fatally; color texture doubles in size if palette exceeds 1024 colors — see inline comment |
| `libs/drape/texture_manager.cpp` | 105 | Arrow texture fallback is `allowOptional=true`; absent file returns null gracefully — see inline comment |
| `libs/drape/texture_manager.cpp` | 497 | `textLanguageIndex` = index into map language list; `kUnsupportedLanguageIndex` = use map default — see inline comment |
| `libs/drape/texture_manager.cpp` | 512 | Mutex serialises HarfBuzz/FreeType (not thread-safe); not a bottleneck in typical browsing — see inline comment |
| `libs/drape/texture_manager.cpp` | 236 | Glyph atlas size not tuned — larger atlas may reduce texture switches |
| `libs/drape/stipple_pen_resource.cpp` | 186 | Known bug tracked at organicmaps/organicmaps#4539 |
| `libs/drape/metal/metal_texture.mm` | 28 | Metal uses `A8Unorm` format for Red channel textures — should be `R8Unorm` but shaders need updating first |
| `libs/drape/glsl_func.hpp` | 5 | `GLM_ENABLE_EXPERIMENTAL` required by current GLM version — remove after upgrading GLM |
| `libs/drape/utils/projection.hpp` | 11 | Historical bug (asymmetric depth bounds caused clip-plane shift) — fixed; bounds are now ±25000 and the offset term cancels |

### Vulkan Backend

| File | Line | Note |
|------|------|------|
| `libs/drape/vulkan/vulkan_base_context.cpp` | 197 | `vkAcquireNextImageKHR` called with infinite timeout — not supported on all Android devices; can deadlock on screen wake |

### Device Workarounds (`support_manager.cpp`)

| File | Line | Note |
|------|------|------|
| `libs/drape/support_manager.cpp` | 95, 103 | PowerVR Rogue GPU workaround — unclear if still needed or if all affected devices should be blocked |
| `libs/drape/support_manager.cpp` | 135 | Route line flickers in navigation mode on some GPUs |
| `libs/drape/support_manager.cpp` | 139 | Dashed lines broke after a `LineShape::Construct<DashedLineBuilder>` update on some devices |
| `libs/drape/support_manager.cpp` | 156 | All Android 10+ (API 29+) assumed to lack Vulkan partial texture update support — may be overly broad |

### Line Rendering

| File | Line | Note |
|------|------|------|
| `libs/drape_frontend/line_shape.cpp` | 323 | Spline filtration done in `LineShape` — should be in `Spline` class |
| `libs/drape_frontend/line_shape.cpp` | 359 | Join/cap artefact reduction factor is empirical (currently `!= 2`) — correct value unclear |
| `libs/drape_frontend/shape_view_params.hpp` | 86 | Default cap/join styles not set — callers must remember to set them |

### Frontend Renderer and DrapeEngine

| File | Line | Note |
|------|------|------|
| `libs/drape_frontend/drape_engine.cpp` | 714 | Direct call to FR bypasses PostMessage due to timing race with the render queue — explained in inline comment |
| `libs/drape_frontend/drape_engine.hpp` | 239 | `CustomFeatures` API is dormant (likely ad-POI origin); retained because re-enabling is trivial — see inline comment |
| `libs/drape_frontend/frontend_renderer.cpp` | 1652 | Semi-opaque subway routing background — unclear if needed |
| `libs/drape_frontend/frontend_renderer.cpp` | 2229 | Overlay set uses `std::set` — `small_set` / `buffer_vector` would be faster for typical sizes |
| `libs/drape_frontend/frontend_renderer.hpp` | 372 | `GetCurrentZoom()` has an assert that `m_currentZoomLevel != -1` — added defensively, root cause of -1 not documented |
| `libs/drape_frontend/render_group.hpp` | 34 | `RenderGroup` has a polymorphic interface that is not actually needed |
| `libs/drape_frontend/render_state_extension.hpp` | 30 | Polymorphic extension mechanism is a design smell; should be a plain struct |
| `libs/drape_frontend/read_manager.cpp` | 227, 231 | Pool avoids per-tile malloc churn (explained); mutex ordering is load-bearing (explained) — see inline comments |

### Miscellaneous

| File | Line | Note |
|------|------|------|
| `libs/drape/drape_global.hpp` | 35 | `dp::Center == 0` means `(anchor & dp::Center)` is always false — anchor arithmetic is inconsistent |
| `libs/drape/pointers.hpp` | 139 | `down_cast` on `ref_ptr` implemented as a member cast — a free function would be preferred but codebase already relies on current form |
| `libs/drape/static_texture.hpp` | 28 | Texture name strings should be `std::string_view` after `StyleReader` refactoring |
| `libs/drape_frontend/drape_global.hpp` | 35 | (same as above) |
| `libs/drape_frontend/my_position_controller.cpp` | 636 | Code block not guarded by `m_hints.m_screenshotMode` — unclear if intentional |
| `libs/drape_frontend/path_text_handle.hpp` | 33 | Both methods are actively used via `PathTextHandle` delegation — see inline comment |
| `libs/drape_frontend/poi_symbol_shape.cpp` | 163 | Real bug: depth value must still be in `[kMinDepth, kMaxDepth]` even when depth test is off or the primitive is clipped — fix is to clamp/remap depth when `depthTestEnabled == false` |
| `libs/drape_frontend/screen_animations.cpp` | 56 | Screen passed intentionally — ensures speed is zoom-independent; see inline comment |
| `libs/drape_frontend/tile_info.cpp` | 87 | Dual cancellation patterns are intentional; throw is the fast path — see inline comment |
| `libs/drape_frontend/visual_params.cpp` | 83 | `mdpi` entry is unreachable on mobile due to platform minimum scale — see inline comment |
| `libs/drape_frontend/gui/ruler_helper.cpp` | 41 | Ruler unit strings not localised |
| `libs/drape_frontend/gui/ruler_helper.cpp` | 53 | Ruler value not fixed to zoom level — can show inconsistent distances at the same zoom |
| `libs/drape_frontend/gui/ruler_helper.cpp` | 205 | Confirmed dead code — ruler width clamping prevents this branch from ever being reached; see inline comment |
