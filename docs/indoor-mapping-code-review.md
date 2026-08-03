# Code Review: `zy-indoor-mapping` branch (full diff vs. main)



## 2. Proximity/geometry correctness and duplication (do these together — same root cause)

Three separate places independently reimplement "expand a rect by ~5m and test if a point/rect is inside," and the degree-based expansion is wrong near the poles (`hackintosh5` gave a concrete Svalbard example: 1.14m in x vs 5.58m in y for the same degree offset):
- `libs/drape_frontend/indoor_filter.hpp` — `kProximityDeg = 0.00005` inline expansion in `ShouldSkipIndoorFeature`.
- `libs/map/indoor_manager.cpp` — `IsNearActiveIndoorContext` (same pattern, added tonight — `hackintosh5` flagged this one too, "duplicated from indoor_filter.hpp").
- Also flagged: recomputing the expanded rect per-feature per-rect in the `ShouldSkipIndoorFeature` loop is wasteful.

**Fix**: add one meters-aware helper — e.g. `bool IsNearAnyRect(m2::PointD const & pt, std::vector<m2::RectD> const & rects, double meters)` — and put it somewhere both `indoor_filter.hpp` and `indoor_manager.cpp` can call (candidate: `libs/indexer/indoor_level.hpp`/`.cpp`, since both files already depend on that header, or a new small header if that's a poor fit). Build the meters→degrees conversion using `mercator::MetersToMercator`/`mercator::RectByCenterXYAndOffset` (`libs/geometry/mercator.hpp:71-91`, confirmed to exist and already used this way in `libs/geometry/geometry_tests/circle_on_earth_tests.cpp:52`) instead of the flat `kProximityDeg` constant — note this is still a fixed-latitude approximation unless the helper takes the point's actual latitude into account; check `RectByCenterXYAndOffset`'s implementation to confirm it already does the correct latitude-aware conversion (`GetSmPoint`, `mercator.hpp:82-91`) before assuming it fixes the pole issue outright. Have callers expand each building's rect(s) once (e.g. when `m_indoorPolygonRects` is set in `ApplyScanResult`) rather than per-feature, to also resolve the performance nitpick.

## 3. Code organization (do these)

### 3a. `TriangleIntersectsRect` → `libs/geometry/triangle2d.hpp`/`.cpp` (`hackintosh5`: "should live in base or geometry")
Confirmed no existing home. It's currently a static free function in `libs/map/indoor_manager.cpp:35-53`, built entirely from existing `geometry/` primitives (`rect.IsPointInside`, `m2::IsPointInsideTriangle`, `m2::Intersect`). Move it to `libs/geometry/triangle2d.hpp`/`.cpp` as `m2::TriangleIntersectsRect` (or similar), update `indoor_manager.cpp` to call the moved version.

### 3b. `libs/drape_frontend/indoor_filter.hpp` — move out of the header (`hackintosh5`)
`ShouldSkipIndoorFeature` is `inline` in a `.hpp` for no stated reason. Create `indoor_filter.cpp`, move the implementation there, drop `inline`. Check `libs/drape_frontend/CMakeLists.txt` for whether a new `.cpp` needs registering (likely yes, alongside the existing `.hpp`/`_tests.cpp` entries).

### 3c. `libs/drape_frontend/drape_engine.hpp` — `SetIndoorLevel` default parameters (`hackintosh5`: "unnecessary and potentially harmful")
```cpp
void SetIndoorLevel(double level, std::vector<double> availableLevels = {},
                     std::vector<m2::RectD> indoorPolygonRects = {});
```
Drop the `= {}` defaults; make callers pass explicit arguments. Check every call site (`indoor_manager.cpp`'s several `SafeCall(&df::DrapeEngine::SetIndoorLevel, ...)` sites) already passes all three — confirm none currently relies on the default before removing it.

### 3d. `libs/map/framework.cpp` — duplicated car-context check (found tonight, same theme as `hackintosh5`'s duplication comments)
`m_isCarScreenMode` + `GetCurrentRouterType() == RouterType::Vehicle` is written out in both `CanEnterPredicate` and `ShouldHoldPredicate`. Extract a small `bool Framework::IsCarNavigationContext() const` (or similar) and use it in both.

## 4. Threading (do the cheap parts, document the rest)

- `hackintosh5` asked for a comment on `IndoorManager` documenting which methods run on which thread — add a short class-level comment to `libs/map/indoor_manager.hpp`.
- Two instances flagged of `IndoorManager` calling into `RoutingManager`/`BookmarkManager` from the UI thread "technically incorrect" but reviewer says won't crash — lowest priority; note in the plan but don't chase unless you want full correctness now. Locate both via the predicate lambdas set up in `framework.cpp`'s `CreateDrapeEngine` and note whether an easy dispatch fix exists; if not obviously cheap, leave as a documented known issue and say so in the PR reply.
- `SafeCall` overuse (multiple comments: unnecessary locking, notifying drape when nothing changed, `!active` case) — audit the `SafeCall(&df::DrapeEngine::SetIndoorLevel, ...)` call sites in `ApplyScanResult`/`UpdateViewport` for cases where the level truly didn't change and the call could be skipped. Treat as an optional performance pass, not correctness.
- `|=` short-circuiting nitpick — trivial, apply opportunistically if touching that line for another fix, not worth a dedicated pass.
- "Can we safely call `ForEachFeature` on `Platform::Thread::File`?" — this is a question, not a requested change. Reply on the PR with a yes/no once confirmed (check what `m_forEachFeature` actually does — it's the `DataSource::ForEachInRect`-style callback wired in `Framework`'s `IndoorManager` construction).

## 5. `std::optional<double>` instead of NaN sentinel — flag as largest single item, needs a scope decision

`hackintosh5` flagged this twice (`indoor_level.hpp`'s `kNoActiveLevel`, and `read_manager.cpp`'s "NaN never compares equal" workaround comment) and called NaN "really quite ugly." This is the biggest single change in scope: `activeLevel` as a `double` (sometimes NaN) flows through `indoor_level.hpp` (`HasActiveLevel`/`kNoActiveLevel`), `IndoorManager::m_activeLevel`, `ShouldSkipIndoorFeature`'s parameter, `DrapeEngine::SetIndoorLevel`, `ReadManager::SetIndoorLevel`, and the cross-thread message that carries it to the render thread. Converting to `std::optional<double>` touches all of those call sites and the message-passing struct.

**Recommend doing this one separately after the smaller fixes land**, or at minimum confirming you want it in this PR at all — it's real churn for a "quite ugly but not incorrect" complaint, versus the other items which are either bugs or cheap wins. Flag this explicitly when presenting the plan rather than just doing it.

## 6. Design questions to reply to on the PR, not fix in code

- `rule_drawer.cpp` "spaghettification" comment on building:part-as-indoor handling — you already gave a concrete rationale (SOTM venue elevated walkway example) in-thread; `hackintosh5` replied "Why?" (terse — probably wants the *code* to say what you said in the comment, not just the PR thread). **Action**: turn your PR-comment explanation into an actual code comment at the relevant `rule_drawer.cpp` site, don't just leave it in the review thread.
- `framework.cpp` "is ShouldHoldPredicate actually useful in practice?" — you already committed to a refactor in-thread; tonight's session work (car-context exclusion, follow-mode-aware speed gate, proximity-based hold) **is** that refactor. **Action**: reply on the PR summarizing what shipped, point at the current `CanEnterPredicate`/`ShouldHoldPredicate` in `framework.cpp`.
- "Modes UI" wish (manual indoor toggle) — bigger feature, out of scope for this PR. Reply acknowledging, no code change.
- Retiling/flicker open question (`hackintosh5`'s own uncertainty about "notify only when polygon rects differ" vs the risk of retile flicker) — cross-reference with `Woozy`'s report of "colors of buildings and road arrows flash slightly when changing levels." Worth a quick investigation (does `ApplyScanResult` currently re-notify drape on every scan even when nothing changed?) before deciding whether to implement `hackintosh5`'s suggestion or reply that it's a known tradeoff.

## 7. Field-testing UX fixes

- **POIs without a level tag look confusing** (`LMBishop`) — add a visual indicator (fade or marker) for level-tagged-adjacent POIs that have no level of their own, while in indoor mode. Touches `data/styles/` mapcss and/or `rule_drawer.cpp`'s indoor-feature handling.
- **Level picker overlaps zoom/compass in landscape** (`LMBishop`, 2 screenshots) — Android layout fix. Look at `android/app/src/main/res/layout*/map_buttons_layout_navigation.xml` / `map_buttons_layout_planning.xml` variants and `MapButtonsController.java` for how the indoor level picker is positioned relative to zoom/compass controls; needs repositioning or dynamic-move logic for landscape.
- **Visual flicker changing levels** (`Woozy`) — likely same root cause as the retiling question in §6; investigate together.
- **Railway/subway platform display** (`ViktorP06`) — you already addressed the filtering side (platforms marked as indoor-equivalent so adjacent rail lines filter correctly). ViktorP06's actual ask was *display* quality ("we can make it look good later"), not routing — treat as a style/polish item, lower priority, confirm scope with him on the PR if picking this up now vs. later.

## Suggested order of work

1. Split iOS to its own branch (§0) — unblocks a clean PR state, matches your answer just now.
2. Cheap, unambiguous correctness fixes (§1a-1d) — small, independent, no design risk.
3. Unify the proximity/duplication logic (§2) — one root cause, three call sites, fixes a real pole-latitude bug.
4. Code organization (§3) — mechanical moves, no behavior change, directly what the reviewer asked for.
5. Threading doc comment + cheap `SafeCall` audit (§4) — quick.
6. Reply to the design-question threads (§6) — no code, just close the loop with the reviewer (folding your rationale into an actual code comment for the spaghettification one).
7. Decide on §5 (`std::optional`) — separate scope call.
8. Field-testing UX fixes (§7) — largest remaining chunk, own pass.

## Verification

- Build after each numbered section (arm64 incremental compile-check, as used all session: `./gradlew assembleWebDebug -Parm64` with a 600000ms timeout).
- Re-run `libs/drape_frontend/drape_frontend_tests/indoor_filter_tests.cpp` and `libs/map/map_tests/indoor_manager_tests.cpp` after §1-§3 (epsilon, NaN-adjacent, and geometry changes all have direct test coverage there already).
- For the meters-aware proximity fix (§2), consider adding a test using coordinates near a pole (mirroring `hackintosh5`'s Svalbard example) to actually prove the latitude-correction works, not just that behavior is unchanged near the equator.
- For UX fixes (§7), manual verification on-device/emulator (screenshot comparison against `LMBishop`'s landscape screenshots for the picker-overlap fix).
- Post PR replies for §6 once corresponding code/comments land, and for the iOS branch split (§0) once the new branch exists.




## Context

The `zy-indoor-mapping` branch adds OSM indoor mapping (level=* filtering, a level
picker, indoor styling, a `?indoor` debug overlay, and routing/test-MWM tooling) across
Android, iOS, Qt, and the shared C++ core. Scope: **29 commits, ~2,856 insertions / 42
deletions across 93 files** (merge-base `555b0866db`). This document is a **written
critique only** — no code changes are made here. It evaluates code style, minimality of
changes, maintainability, crash/bug risk, and correctness, and gives prioritized
recommendations. Decision on record: the LERROR→LWARNING downgrades (Finding H1) should be
fully reverted and root-caused.

## Overall assessment

The core feature is **well-architected**: `IndoorManager` cleanly mirrors the existing
`IsolinesManager` pattern, the drape plumbing follows established conventions, the level
parser and filter are pure/testable and have real unit tests (`indoor_level_tests`,
`indoor_filter_tests`, `indoor_manager_tests`, iOS `IndoorManagerTests`). Comments are
unusually thorough and explain *why*, not just *what*. The main problems are not in the
feature design but in **debug/test scaffolding and a broad error-handling band-aid that
must not ship**, plus a handful of correctness edge cases and production log noise.

---

## High severity (must fix before merge)

### H1 — LERROR→LWARNING downgrades degrade production observability (10 files)
`libs/routing/{index_graph_loader,index_router,city_roads,maxspeeds}.cpp`,
`libs/search/{house_to_street_table,lazy_centers_table}.cpp` (and related).
The branch downgrades genuine error logs for **unrelated** subsystems — speed cameras,
road access, road penalties, house-to-street, centers table — to work around a debug-build
abort caused by *incomplete test MWMs*. This is a band-aid with a large blast radius: it
silences real errors for every production user with a corrupt/partial map, not just the
test scenario. Several sites (`index_graph_loader` `throw;`) make the level purely
cosmetic anyway.
**Recommendation (on record): revert all 10 downgrades.** Solve the debug-abort at
the source — generate complete test MWMs (the branch already adds the `--make_cross_mwm`
pass and gen scripts), and/or gate the abort behavior so a missing optional section in a
test MWM doesn't abort a debug build. Do not couple production error severity to test-data
completeness.

### H2 — `mall_diag_test.cpp` is a scratch diagnostic bound to a non-repo MWM
`libs/map/map_tests/mall_diag_test.cpp` (registered in `map_tests/CMakeLists.txt`).
`UNIT_TEST(MallDiag_IndoorScan)` hardcodes `RegisterMap(... "MallOfAmerica")` and an
emulator-observed viewport rect. `MallOfAmerica.mwm` is an untracked local file (it shows
up in `git status` as untracked, not committed), so in CI this test has no data and will
fail or silently no-op — a flaky/meaningless test. It's a diagnostic used during
development, not an assertion of correct behavior.
**Recommendation: delete the file and its CMakeLists entry.** If any of its checks are
worth keeping, fold them into `indoor_manager_tests.cpp` using a synthetic/committed
fixture, not a bundled binary MWM.

### H3 — Verbose `LINFO` logging in hot paths
`libs/map/indoor_manager.cpp` lines 107, 140, 146, 241, 262, 275 (plus JNI
`IndoorManager.cpp`). `UpdateViewport` logs `LINFO` on **every viewport change**;
`GetViewportLevels`/`GetActiveLevel` log on every UI query; `ScheduleScan`/`ApplyScanResult`
log per scan. At `LINFO` these ship in release and spam the log during normal panning/zoom.
**Recommendation: remove or demote to `LDEBUG`.** Keep at most the one-time
"indoor mode triggered by…" line, and even that at `LDEBUG`.

---

## Medium severity (should fix)

### M1 — Unbounded level-range expansion in the parser
`libs/indexer/indoor_level.cpp` `ParseLevels` → `ParseRange`: a token like `0-100000`
expands to 100k+ `double`s in a `for (l=from; l<=to; l+=1.0)` loop. Input comes from
untrusted OSM `level=*` tags. Memory/CPU spike (and float-accumulation drift on long
loops) for a malformed tag.
**Recommendation: cap the expanded count (e.g. reject ranges wider than ~100 floors) and
prefer integer stepping.**

### M2 — `GetFeatureAtPoint`: `line` still outranks `indoorArea`
`libs/map/framework.cpp` return ladder: `poi > line > indoorArea > area`. In indoor mode,
tapping a room that a footway/aisle line crosses (within ~3 m) selects the **line**, not
the room.
**Recommendation: decide deliberately.** If rooms should win indoors, rank `indoorArea`
above `line` when `m_indoorManager.IsActive()`. Document whichever is chosen.

### M3 — Indoor-mode area tap can now select non-building areas
`libs/map/framework.cpp` `BuildPlacePageInfo`: in indoor mode it calls `GetFeatureAtPoint`
(which falls back to *any* closest area) instead of `FindBuildingAtPoint` (buildings only).
When no indoor feature is under the tap, it may now return a landuse/park polygon and, via
`isBuildingSelected == false`, place the selection circle at that area's center rather than
the tap point — a subtle behavior change vs. outdoor taps.
**Recommendation: in indoor mode, only accept the `GetFeatureAtPoint` result if it is an
indoor feature; otherwise fall through to `FindBuildingAtPoint`** (preserves prior building
semantics).

### M4 — `apply_feature_functors`: outline generalization widens a hot path
`ApplyAreaFeature::NeedOutline` returns true for *any* area rule with a differing border
color + width>0, routing it through `ProcessBuildingPolygon`, which is unclipped and uses
`GetIndex` (O(n²) in vertices, `buffer_vector<…, kBuildingOutlineSize>` spilling to heap
for large polys). Fine for small indoor rooms/dams today, but a future casing on a large
landcover area would silently hit the quadratic/unclipped path.
**Recommendation: acceptable now; add a brief comment noting the assumption ("intended for
small polygons"), or guard with a vertex-count/size ceiling** so a future style change
can't regress rendering perf.

### M5 — `ApplyScanResult` drops fresh polygon rects when the level set is unchanged
`libs/map/indoor_manager.cpp`: `if (levels == m_levels) return;` early-returns before
`m_indoorPolygonRects` is updated. If the same level set persists but the building/polygons
shifted, proximity rects go stale.
**Recommendation: minor — refresh `m_indoorPolygonRects` even when `levels` is unchanged, or
document that identical level sets imply the same building.**

---

## Low severity (style / maintainability / polish)

- **L1 — Equator-only meter comments.** `indoor_filter.hpp` (`kProximityDeg 0.00005 ≈ 5 m`),
  `indoor_manager.cpp` (`kMinHalf ≈ 55 m`, `kMaxTapRadius ≈ 110 m`), `framework.cpp`
  (`kMinBoxHalf ≈ 10 m`) treat mercator degrees as constant meters. At MoA/Paris latitudes
  the true distances are ~30% smaller in longitude. Fine for tap targets/debug; note the
  approximation or convert via `mercator::` helpers where precision matters.
- **L2 — Large inline debug lambda.** The `SetDebugRectsListener` body in
  `framework.cpp::CreateDrapeEngine` is ~60 lines of debug-overlay drawing embedded in a
  production init method. Extract to a named private method (e.g. `DrawIndoorDebugRects`)
  for readability.
- **L3 — `GetDebugFeatureAt` ranks by distance to rect *center*.** For a large mall rect,
  a tap inside it but nearer another feature's center mis-identifies. Debug-only; consider
  point-in-rect first, then center distance.
- **L4 — Magic numbers.** Debug color table, alpha 200/90, widths, `kColorCount = 7`,
  `kMaxIndoorRectDeg = 0.1` are reasonable but undocumented constants; a short rationale
  comment each aids future maintainers (most already have one — apply consistently).
- **L5 — Duplicated "isLeveled" logic.** The building/building-part exclusion for
  level-tagged features is repeated in `indoor_filter.hpp`, `indoor_manager.cpp`
  (`ScheduleScan`), and implied elsewhere. Consider a shared helper to avoid drift.
- **L6 — `FormatLevel` fractional path** uses default `ostringstream` precision; verify
  half-levels (e.g. `0.5`) render as intended across locales (no locale-specific decimal).

---

## What's done well (keep)

- Clean manager pattern mirroring `IsolinesManager`; correct generation-counter cancellation
  for async viewport scans; `std::atomic` where cross-thread; mutex-guarded debug rects for
  JNI access.
- Pure, unit-tested `ParseLevels`/`LevelsContain`/`ShouldSkipIndoorFeature`; good negative
  cases (transit platform at level −1 near a 0–3 mall stays visible).
- The tap-selection root-cause fix (`5dbc741005`) correctly identifies that drape reports
  only overlay/POI taps and wires indoor-area selection through the framework — the right
  layer.
- The area-casing fix (`a681aa174a`) fulfills a long-standing in-code TODO
  ("Make borders work for non-building areas too") rather than hacking the style layer.
- Thorough, intent-explaining comments throughout; consistent commit prefixes and DCO.

---

## Suggested remediation order (for a future implementation pass)

1. Revert H1 (10 files) + root-cause the debug abort via complete test MWMs / debug guard.
2. Delete H2 (`mall_diag_test.cpp`) and its CMake entry.
3. Strip/demote H3 logging.
4. M1 range cap; M2/M3 indoor tap precedence decisions; then M4/M5 and the L-items.

## Verification (once fixes land)

- `drape_frontend_tests` (IndoorFilter), `indexer_tests` (indoor_level),
  `map_tests` (indoor_manager) all green; confirm no test depends on an untracked MWM.
- Build Android + a routing-capable desktop/CI config **in debug**; confirm no LERROR abort
  with a *complete* test MWM and that reverted LERROR logs still surface on genuinely
  corrupt data.
- Grep the shipped log at `LINFO` during pan/zoom over an indoor building — should be quiet.
- Manual: indoor tap selects room (not building/footway per M2 decision); non-indoor area
  tap outside indoor mode unchanged.
