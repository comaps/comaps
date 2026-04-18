"""Dump feature types present in the routing section of an MWM file.

The routing section stores bit-packed, gamma-coded feature IDs grouped by
VehicleMask (pedestrian / bicycle / car).  This tool parses that section,
cross-references with the features section to recover type names, and reports
the type distribution – optionally filtered by a substring.

Binary format reference: libs/routing/index_graph_serialization.hpp
Bit-stream reference:    libs/coding/bit_streams.hpp
Coder reference:         libs/coding/elias_coder.hpp
"""

import struct
from collections import defaultdict
from typing import Dict, List, Optional

from mwm.mwm_python import MwmPython
from mwm.feature_types import readable_type

# Vehicle mask bits (routing/vehicle_mask.hpp)
PEDESTRIAN_MASK = 0x01
BICYCLE_MASK    = 0x02
CAR_MASK        = 0x04


def _mask_str(mask: int) -> str:
    parts = []
    if mask & PEDESTRIAN_MASK: parts.append("pedestrian")
    if mask & BICYCLE_MASK:    parts.append("bicycle")
    if mask & CAR_MASK:        parts.append("car")
    return "+".join(parts) if parts else "none"


# ---------------------------------------------------------------------------
# Bit-stream reader – LSB-first, matching coding/bit_streams.hpp BitReader
# ---------------------------------------------------------------------------

class _BitReader:
    def __init__(self, data: bytes):
        self._data = data
        self._pos = 0
        self._buf = 0
        self._buffered = 0  # valid bits in _buf

    def read(self, n: int) -> int:
        """Read n bits (n ≤ 8), returned as the low bits of the result."""
        if n == 0:
            return 0
        if n <= self._buffered:
            result = self._buf & ((1 << n) - 1)
            self._buf >>= n
            self._buffered -= n
            return result
        next_byte = self._data[self._pos]
        self._pos += 1
        low = n - self._buffered
        result = ((next_byte & ((1 << low) - 1)) << self._buffered) | self._buf
        self._buf = next_byte >> low
        self._buffered = 8 - low
        return result

    def read_bits(self, n: int) -> int:
        """Read up to 64 bits, assembled in LSB-first 8-bit chunks."""
        result = 0
        shift = 0
        while n > 0:
            take = min(n, 8)
            result |= self.read(take) << shift
            shift += take
            n -= take
        return result


def _decode_gamma(r: _BitReader) -> int:
    """Elias-gamma decode (GammaCoder::Decode)."""
    n = 0
    while r.read(1) == 0:
        n += 1
    return (1 << n) | r.read_bits(n)


def _decode_delta(r: _BitReader) -> int:
    """Elias-delta decode (DeltaCoder::Decode)."""
    n = _decode_gamma(r) - 1
    return (1 << n) | r.read_bits(n)


def _convert_joints(encoded: int) -> int:
    """IndexGraphSerializer::ConvertJointsNumber – swaps 1↔2 for Gamma efficiency."""
    if encoded == 1: return 2
    if encoded == 2: return 1
    return encoded


# ---------------------------------------------------------------------------
# Routing section parser
# ---------------------------------------------------------------------------

def _parse_routing_section(data: bytes) -> Dict[int, List[int]]:
    """Return {vehicle_mask: [feature_id, ...]} from raw routing section bytes."""
    if len(data) < 13:
        return {}

    pos = 0

    def u8():
        nonlocal pos
        v = data[pos]; pos += 1; return v

    def u32():
        nonlocal pos
        v = struct.unpack_from('<I', data, pos)[0]; pos += 4; return v

    def u64():
        nonlocal pos
        v = struct.unpack_from('<Q', data, pos)[0]; pos += 8; return v

    version = u8()
    if version != 0:
        raise ValueError(f"Unknown routing section version: {version}")

    _num_roads  = u32()  # total across all sections (informational)
    _num_joints = u32()
    num_sections = u32()

    sections = []
    for _ in range(num_sections):
        size      = u64()
        n_roads   = u32()
        begin_jid = u32()
        end_jid   = u32()
        mask      = u32()
        sections.append((size, n_roads, begin_jid, end_jid, mask))

    mask_to_fids: Dict[int, List[int]] = {}

    for (size, n_roads, _begin, _end, mask) in sections:
        section_bytes = data[pos: pos + size]
        pos += size

        fids: List[int] = []
        reader = _BitReader(section_bytes)
        fid = 0xFFFFFFFF  # uint32 -1 (ring arithmetic)

        for _ in range(n_roads):
            delta = _decode_gamma(reader)
            fid = (fid + delta) & 0xFFFFFFFF
            fids.append(fid)

            joints = _convert_joints(_decode_gamma(reader))

            pt_id = 0xFFFFFFFF  # uint32 -1
            for _ in range(joints):
                pt_delta = _decode_gamma(reader)
                pt_id = (pt_id + pt_delta) & 0xFFFFFFFF
                # Joint ID: 0 bit = new (skip), 1 bit = repeat (skip delta)
                if reader.read(1) == 1:
                    _decode_delta(reader)

        mask_to_fids.setdefault(mask, []).extend(fids)

    return mask_to_fids


# ---------------------------------------------------------------------------
# Public entry point
# ---------------------------------------------------------------------------

def analyze_routing(path: str, type_filter: Optional[str] = None, show_features: bool = False):
    """Dump feature types found in the routing section.

    Args:
        path:         Path to the .mwm file.
        type_filter:  If given, only report types whose name contains this substring.
        show_features: If True, list individual feature IDs for each type.
    """
    mwm = MwmPython(path)
    sections = mwm.sections_info()

    print(f"File:       {mwm.name()}")
    print(f"Map type:   {mwm.type()}")
    print(f"MWM format: {mwm.version().format}")
    print()

    routing_info = sections.get("routing")
    if routing_info is None or routing_info.size == 0:
        print("No routing section present.")
        return

    print(f"Routing section: offset={routing_info.offset}, size={routing_info.size} bytes")
    print()

    routing_bytes = bytes(mwm.file[routing_info.offset: routing_info.offset + routing_info.size])

    try:
        mask_to_fids = _parse_routing_section(routing_bytes)
    except Exception as exc:
        print(f"ERROR parsing routing section: {exc}")
        raise

    if not mask_to_fids:
        print("Routing section is empty — no routable roads found.")
        return

    total_entries = sum(len(v) for v in mask_to_fids.values())
    print(f"Routable road entries:  {total_entries}")
    print(f"Vehicle mask groups:    {len(mask_to_fids)}")
    print()

    # Index all features by their sequential ID so we can look up types.
    print("Indexing features...")
    fid_to_types: Dict[int, List[str]] = {}
    for ft in mwm:
        fid_to_types[ft.index()] = [readable_type(t) for t in ft.types()]
    print(f"  {len(fid_to_types)} features indexed.")
    print()

    # Aggregate type counts per mask and globally.
    mask_type_counts: Dict[int, Dict[str, int]] = {}
    global_type_count: Dict[str, int] = defaultdict(int)
    # For --show_features: type -> list of fids
    type_to_fids: Dict[str, List[int]] = defaultdict(list)

    for mask, fids in mask_to_fids.items():
        tc: Dict[str, int] = defaultdict(int)
        for fid in fids:
            for type_name in fid_to_types.get(fid, ["<unknown>"]):
                if type_filter is None or type_filter in type_name:
                    tc[type_name] += 1
                    global_type_count[type_name] += 1
                    if show_features:
                        type_to_fids[type_name].append(fid)
        mask_type_counts[mask] = tc

    # Per-mask breakdown
    for mask in sorted(mask_to_fids):
        fids = mask_to_fids[mask]
        tc = mask_type_counts[mask]
        filtered_count = sum(tc.values())
        if type_filter and filtered_count == 0:
            continue
        print(f"=== Mask 0x{mask:02X} ({_mask_str(mask)}) — {len(fids)} road(s)"
              + (f", {filtered_count} type-hits" if type_filter else "") + " ===")
        if not tc:
            print("  (no types match filter)" if type_filter else "  (no type data)")
        else:
            for type_name, count in sorted(tc.items(), key=lambda x: -x[1]):
                print(f"  {count:>6}  {type_name}")
        print()

    # Global summary
    header = "Types in routing section"
    if type_filter:
        header += f" matching '{type_filter}'"
    print(f"=== {header} ===")
    if not global_type_count:
        print("  None found.")
    else:
        for type_name, count in sorted(global_type_count.items(), key=lambda x: -x[1]):
            print(f"  {count:>6}  {type_name}")
            if show_features:
                fids_for_type = type_to_fids[type_name]
                preview = fids_for_type[:20]
                suffix = f" ... (+{len(fids_for_type)-20} more)" if len(fids_for_type) > 20 else ""
                print(f"          feature ids: {preview}{suffix}")
    print()
    print(f"Total: {sum(global_type_count.values())} type occurrences across {total_entries} road entries.")
