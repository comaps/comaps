# MWM File Format Introduction

The `.mwm` (MapsWithMe, a predecessor of Maps.Me, Organic Maps, and CoMaps) binary
format stores offline map tiles for rendering, search, and routing. Each file
represents a geographic region (country/state), with two special global files:
`World.mwm` (low-detail base map) and `WorldCoasts.mwm` (coastlines).
File versions are encoded as `YYMMDD` integers (e.g. `260106`).

Source references point to the `libs/` subtree unless otherwise noted.

## Container Structure

An MWM file is a flat archive of tagged binary sections. Implemented in
`coding/files_container.hpp`.

```
Offset  Size     Contents
──────  ───────  ──────────────────────────────────────
0       8        uint64 offset to section table (T)
8       varies   Section data (each aligned to 8 bytes)
...
T       varies   Section table:
                   vector<TagInfo> serialized as:
                     VarUint  count
                     for each entry:
                       String   tag  (length-prefixed UTF-8)
                       VarUint  offset from file start
                       VarUint  size in bytes
```

Reading: read 8 bytes at offset 0 to locate the table, then iterate entries.
Each section is addressed by `[offset, offset+size]`. Sections are sorted by tag
for binary search lookup.

## Section Directory

All tag names defined in `defines.hpp`:

| Tag | Section | Description |
|---|---|---|
| `version` | Format version | uint8 format enum + uint64 seconds-since-epoch |
| `header` | Data header | Bounds, scales, geometry coding params, map type |
| `features` | Feature records | Variable-length feature data (was `dat` before v10) |
| `geom` | Line geometry | Polyline coordinates at multiple scales (`geom0`..`geom3`) |
| `trg` | Area geometry | Triangle strips at multiple scales (`trg0`..`trg3`) |
| `idx` | Scale index | Spatial index for scale-bucketed feature lookup |
| `sdx` | Search index | Trie mapping text tokens to feature IDs |
| `centers` | Feature centers | Encoded center points for quick access |
| `offs` | Feature offsets | Elias-Fano encoded offset table for O(1) feature lookup |
| `meta` | Metadata | Compressed key-value metadata (phone, website, etc.) |
| `addr` | Street addresses | Feature-to-street mapping |
| `ft2place` | Place mapping | Feature-to-place mapping |
| `postcodes` | Postcodes | Postal code data |
| `postcode_points` | Postcode points | Postcode location points |
| `ranks` | Search ranks | Search result ranking scores |
| `popularity` | Popularity | Popularity scores |
| `cities_boundaries` | City bounds | City boundary polygons |
| `altitudes` | Elevation | Altitude data for features |
| `routing` | Route graph | Road routing graph |
| `cross_mwm` | Cross-MWM routing | Routing connections between adjacent MWM files |
| `routing_world` | World routing | World-level routing data |
| `restrictions` | Turn restrictions | Routing turn/access restrictions |
| `roadaccess` | Road access | Road access restriction flags |
| `roadpenalty` | Road penalties | Turn penalty weights |
| `maxspeeds` | Speed limits | Maximum speed data |
| `city_roads` | City roads | City road classification |
| `metalines` | Metalines | Joined way rendering data |
| `speedcams` | Speed cameras | Speed camera locations |
| `traffic` | Traffic | Live traffic key data |
| `transit` | Public transit | Public transport routes and stops |
| `transit_cross_mwm` | Cross-MWM transit | Transit connections between MWMs |
| `isolines_info` | Isolines | Topographic contour metadata |
| `descriptions` | Descriptions | Wikipedia/feature descriptions |
| `rgninfo` | Region info | Regional information |
| `relations` | Relations | Feature relation data |
| `rel_offs` | Relation offsets | Relation offset table |

Not all sections are present in every file. `World.mwm` and `WorldCoasts.mwm`
omit routing/transit sections. The `feature_to_osm` tag exists only in
intermediate build artifacts, not production files.

## Version Section

Tag: `version`. See `platform/mwm_version.hpp`.

```
Offset  Size  Contents
──────  ────  ──────────────────────────────
0       1     uint8  Format enum (see below)
1       8     uint64 Seconds since Unix epoch
```

Format versions:

| Enum | Value | Date | Key change |
|---|---|---|---|
| v1 | 0 | 2011-04 | Initial format |
| v2 | 1 | 2011-11 | Type index instead of raw type |
| v3 | 2 | 2013-03 | Type index in search data |
| v4 | 3 | 2015-04 | Distinguish и/й in search |
| v5 | 4 | 2015-07 | Feature ID = index in vector |
| v6 | 5 | 2015-10 | Offsets vector in MWM |
| v7 | 6 | 2015-11 | Multiple search index formats |
| v8 | 7 | 2016-02 | Long metadata strings; epoch timestamps |
| v9 | 8 | 2017-04 | OSRM replaced by cross-MWM routing |
| v10 | 9 | 2020-04 | Section headers; compressed metadata index; `dat`→`features` |
| v11 | 10 | 2020-09 | Compressed string storage for metadata |

## Data Header Section

Tag: `header`. See `indexer/data_header.hpp`.

Serialized sequentially (no fixed offsets — all fields are varint-encoded):

```
Field                        Encoding
───────────────────────────  ──────────────────────────
GeometryCodingParams:
  coordBits                  VarUint (typically 27-30)
  basePointUint64            VarUint (packed m2::PointU)
bounds.first                 VarInt (lower-left corner)
bounds.second                VarInt (upper-right corner)
scaleCount                   VarUint
scales[scaleCount]           uint8 each
langCount                    VarUint
langs[langCount]             uint8 each (currently unused)
mapType                      VarInt (0=World, 1=WorldCoasts, 2=Country)
```

Scale arrays from `indexer/feature_impl.hpp`:
- World: `[3, 5, 7, 9]`
- Country: `[10, 12, 14, 17]`

Geometry is stored at each scale level independently, enabling progressive
level-of-detail rendering. See `indexer/scales.hpp`.

## Features Section

Tag: `features`. See `indexer/dat_section_header.hpp`, `indexer/feature.cpp`.

### Section Header (v10+)

```
Offset  Size  Contents
──────  ────  ──────────────────────
0       1     uint8   DatSectionHeader::Version (V0=0, V1=1)
1       4     uint32  featuresOffset (relative to section start)
5       4     uint32  featuresSize
```

### Feature Records

After the header, features are stored as variable-length records:

```
For each feature:
  VarUint   recordSize
  uint8[]   feature data (recordSize bytes)
```

### Feature Binary Layout

Each feature record (documented in `docs/feature_structure.md`,
`indexer/feature_data.hpp`):

**Byte 0 — Header:**

```
Bit  Field
───  ─────────────────────────────────────
0-2  TypeCount − 1 (0..7 → 1..8 types)
3    HasName
4    HasLayer
5-6  GeomType: 00=Point, 01=Line, 10=Area, 11=PointEx
7    HasAddInfo
```

**Subsequent fields (order matters):**

1. **Types** — `TypeCount` VarUint classificator indices
2. **Name** (if HasName) — `StringUtf8Multilang` (see `coding/string_utf8_multilang.hpp`):
   `VarUint(totalByteCount - 1)` followed by `totalByteCount` raw bytes.
   The raw bytes are concatenated `(langByte)(UTF-8 chars)` pairs with no
   per-language length prefix or count. `langByte = 0x80 | (langCode & 0x3F)`,
   using the `10xxxxxx` bit pattern (a UTF-8 continuation byte) as a sentinel:
   when this byte appears where a leading byte is expected, it marks a new
   language. 64 language codes supported (see `kMaxSupportedLanguages`).
3. **Layer** (if HasLayer) — `int8` in range `[-10, +10]`
4. **AddInfo** (if HasAddInfo), depends on GeomType:
   - Point: `uint8 rank` (population as log base 1.1)
   - Line: `ref` — `VarUint(len - 1)` + `len` UTF-8 bytes (road number, e.g. "A1")
   - Area/PointEx: `houseNumber` — `StringNumericOptimal`: `VarUint` where
     LSB=1 means the remaining bits are a numeric value, LSB=0 means the
     remaining bits are `(stringLen - 1)` followed by `stringLen` UTF-8 bytes
5. **Point center** (Point/PointEx only) — `VarUint64` encoding a delta-coded
   center point via `coding::EncodePointDeltaAsUint`. In V1+, the LSB is a
   `hasRelations` flag (the point delta is shifted left by 1 bit).
6. **Geometry header 2** (Line/Area only) — bit-packed via `BitSource`
   (see `indexer/feature.cpp:ParseHeader2`). Format depends on
   `DatSectionHeader::Version`:
   - **V0**: 4 bits `elemsCount`. If `elemsCount == 0` (outer geometry),
     next 4 bits are a scale offset mask. Total: 1 byte.
   - **V1+**: 4 bits `elemsCount`, 1 bit `isOuter`, 1 bit `hasRelations`.
     If `isOuter`, `elemsCount` is reinterpreted as the scale offset mask.
     Total: 1 byte (rounded up from 6 bits).
   `elemsCount > 0` means **inner** (inline) geometry; `== 0` means **outer**
   (geometry stored in `geom`/`trg` sections, addressed by offset array).
7. **Inline geometry** (inner) or **offset array** (outer) pointing into
   `geom`/`trg` sections. For inner lines: simplification mask bytes
   (`(elemsCount - 2 + 3) / 4` bytes, 2 bits per non-endpoint) followed by
   delta-encoded points. For inner areas: `elemsCount + 2` triangle strip
   vertices. For outer: one `VarUint32` offset per set bit in the scale mask.

## Geometry and Triangle Sections

Tags: `geom`, `trg`. Suffixed `0`..`3` for each scale index.
See `coding/geometry_coding.hpp`.

Polyline points and triangle strip vertices are delta-encoded:

```cpp
// For each point after the first:
dx = current.x - predicted.x   // int32
dy = current.y - predicted.y   // int32
WriteVarInt(sink, dx);
WriteVarInt(sink, dy);
```

Prediction functions reduce delta magnitude for better varint compression:
- **2-point prediction**: next ≈ 2·p₁ − p₂ (linear extrapolation)
- **3-point prediction**: Catmull-Rom-like from p₁, p₂, p₃

Triangle strips (area geometry) are written per-strip: `VarUint pointCount`
followed by delta-encoded vertices.

### Coordinate System

From `coding/point_coding.hpp`:

- Coordinates are unsigned 32-bit `m2::PointU(x, y)` in Mercator projection
- `kPointCoordBits = 30` — ~10⁹ discrete values per axis
- `kMwmPointAccuracy = 1e-5` degrees — ~1 meter precision
- Mercator x: `[-180, 180]`, y: `[-85.05, 85.05]`
- Conversion: `PointD ↔ PointU` via `coding/point_coding.hpp`

## Feature Offsets Section

Tag: `offs`. See `indexer/features_offsets_table.hpp`.

An Elias-Fano encoded monotone sequence mapping feature index → byte offset
within the `features` section. Provides O(1) random access by feature ID with
minimal storage overhead (succinct data structure from `3party/succinct/`).

## Metadata Section

Tag: `meta`. See `indexer/metadata_serdes.hpp`.

### Header (v10+)

```
Offset  Size  Contents
──────  ────  ────────────────────────────
0       1     uint8   Version (V0=0)
1       4     uint32  stringsOffset
5       4     uint32  stringsSize
9       4     uint32  metadataMapOffset
13      4     uint32  metadataMapSize
```

### Body

Two sub-components:

1. **String storage** (`coding/text_storage.hpp`) — block-compressed text pool.
   Each metadata value is stored once and referenced by a uint32 ID.
2. **Metadata map** (`coding/map_uint32_to_val.hpp`) — maps `featureId →
   vector<pair<uint8 metaType, uint32 stringId>>`. Meta types defined in
   `indexer/feature_meta.hpp` (e.g. phone, website, opening_hours, cuisine).

## Search Index Section

Tag: `sdx`. Trie (prefix tree) mapping tokenized feature names to feature IDs.
Supports transliteration and fuzzy matching. Built by the generator's
`SearchIndexBuilder`. Structure: compressed trie with variable-length keys and
packed feature ID lists.

## Coding Primitives

Used throughout the format. See `coding/varint.hpp`.

**VarUint** — unsigned LEB128:
```
While value > 0x7F:
  emit (value & 0x7F) | 0x80    // 7 data bits + continuation
  value >>= 7
emit value & 0x7F               // final byte, no continuation
```

**VarInt** — signed via ZigZag then VarUint:
```
encoded = (value << 1) ^ (value >> 63)   // map negatives to odd positives
```

**String** — `VarUint length` followed by `length` UTF-8 bytes.

## Generation Pipeline

The `generator/` directory (and `tools/python/maps_generator/`) implements the
full pipeline from OSM PBF to `.mwm`:

1. **OSM parsing** — extract nodes, ways, relations into intermediate format
2. **Feature collection** — `FeatureBuilder` objects with typed geometry
3. **Region clipping** — split features by country/region borders
4. **Geometry simplification** — per-scale Douglas-Peucker simplification
5. **Triangulation** — area features tessellated into triangle strips
6. **Serialization** — `FeatureBuilder::SerializeForMwm()` writes feature records
7. **Index building** — spatial index, search trie, offsets table
8. **Section assembly** — `FilesContainerW` writes all sections into the final `.mwm`

Entry point: `generator/generator_tool/generator_tool.cpp` (C++ CLI).
Python orchestration: `tools/python/maps_generator/` (see its `README.md`).

Key generator flags (partial list):
- `--generate_features` — step 2-3
- `--generate_geometry` — step 4-5
- `--generate_index` — step 7
- `--generate_search_index` — search trie
- `--unpack_mwm` — extract each section to `<path>.<tag>` files

## Inspection Tools

- **`generator_tool --unpack_mwm`** — extracts all sections as separate files
  (`generator/unpack_mwm.cpp`)
- **`generator_tool --dump_types`** — prints feature type statistics
  (`generator/dumper.cpp`)
- **Python bindings** — `tools/python/` contains `mwm/` package for reading MWM
  files from Python (see `docs/building_pybindings.txt`)
