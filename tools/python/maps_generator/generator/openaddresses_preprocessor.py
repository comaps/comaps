#!/usr/bin/env python3
"""Converts an OpenAddresses ZIP to CoMaps .tempaddr address files.

Each address is assigned to a MWM region by point-in-polygon lookup against
the .poly border files in the CoMaps borders directory.  This correctly handles
countries where provinces/states are split into multiple MWM regions (e.g.
Canada_British_Columbia_Vancouver vs Canada_British_Columbia_Northeast).

Usage:
    python3 openaddresses_preprocessor.py <zip_file> <output_dir> --borders-dir <path>

The borders directory should contain the .poly files from the CoMaps repository
(data/borders/).  Shapely is required for spatial operations.
"""

import argparse
import io
import json
import logging
import math
import os
import zipfile

from shapely.geometry import Point, Polygon
from shapely.strtree import STRtree

logger = logging.getLogger("maps_generator")

_COORD_BITS: int = 30
_COORD_SIZE: float = (1 << _COORD_BITS) - 1

_MIN_X: float = -180.0
_MAX_X: float = 180.0
_MIN_Y: float = -180.0
_MAX_Y: float = 180.0

_INTERPOL_NONE: int = 0

_ADDR_EXT: str = ".tempaddr"


def _write_varuint(f: io.RawIOBase, value: int) -> None:
    value = int(value) & 0xFFFFFFFFFFFFFFFF
    while value > 127:
        f.write(bytes([(value & 127) | 128]))
        value >>= 7
    f.write(bytes([value]))


def _zigzag_encode(v: int) -> int:
    v = int(v)
    return ((v << 1) ^ (v >> 63)) & 0xFFFFFFFFFFFFFFFF


def _write_varint(f: io.RawIOBase, value: int) -> None:
    _write_varuint(f, _zigzag_encode(value))


def _write_string(f: io.RawIOBase, s: str) -> None:
    encoded = s.encode("utf-8")
    _write_varuint(f, len(encoded))
    if encoded:
        f.write(encoded)


def _lat_to_y(lat: float) -> float:
    lat = max(-86.0, min(86.0, lat))
    sinx = math.sin(math.radians(lat))
    res = math.degrees(0.5 * math.log((1.0 + sinx) / (1.0 - sinx)))
    return max(_MIN_Y, min(_MAX_Y, res))


def _double_to_uint32(x: float, min_v: float, max_v: float) -> int:
    if x <= min_v:
        d = 0.0
    elif x >= max_v:
        d = _COORD_SIZE
    else:
        d = (x - min_v) / (max_v - min_v) * _COORD_SIZE
    return int(0.5 + d)


def _perfect_shuffle(x: int) -> int:
    x &= 0xFFFFFFFFFFFFFFFF
    x = ((x & 0x00000000FFFF0000) << 16) | ((x >> 16) & 0x00000000FFFF0000) | (x & 0xFFFF00000000FFFF)
    x = ((x & 0x0000FF000000FF00) << 8)  | ((x >> 8)  & 0x0000FF000000FF00) | (x & 0xFF0000FFFF0000FF)
    x = ((x & 0x00F000F000F000F0) << 4)  | ((x >> 4)  & 0x00F000F000F000F0) | (x & 0xF00FF00FF00FF00F)
    x = ((x & 0x0C0C0C0C0C0C0C0C) << 2)  | ((x >> 2)  & 0x0C0C0C0C0C0C0C0C) | (x & 0xC3C3C3C3C3C3C3C3)
    x = ((x & 0x2222222222222222) << 1)  | ((x >> 1)  & 0x2222222222222222) | (x & 0x9999999999999999)
    return x & 0xFFFFFFFFFFFFFFFF


def _point_to_int64(lon: float, lat: float) -> int:
    ux = _double_to_uint32(lon,            _MIN_X, _MAX_X)
    uy = _double_to_uint32(_lat_to_y(lat), _MIN_Y, _MAX_Y)
    return int(_perfect_shuffle((uy << 32) | ux))


def _parse_poly(path: str) -> list[list[tuple[float, float]]]:
    """Parse an OSM .poly file into a list of rings.

    Each ring is a list of (lon, lat) tuples.  Rings whose index line starts
    with '!' are holes and are returned as-is (callers decide how to handle
    them; for point-in-polygon purposes holes are ignored — a point inside a
    hole is still inside the outer polygon for MWM assignment).
    """
    rings: list[list[tuple[float, float]]] = []
    current: list[tuple[float, float]] | None = None
    with open(path, encoding="utf-8") as fh:
        for i, line in enumerate(fh):
            stripped = line.strip()
            if i == 0:
                continue
            if stripped == "END":
                if current is not None:
                    rings.append(current)
                    current = None
                continue
            if stripped.lstrip("!").isdigit():  # "!N" = hole ring
                current = []
                continue
            if current is not None and stripped:
                parts = stripped.split()
                if len(parts) >= 2:
                    try:
                        lon = float(parts[0])
                        lat = float(parts[1])
                        current.append((lon, lat))
                    except ValueError:
                        pass
    return rings


class _BorderIndex:
    """Spatial index over .poly border files for point-in-polygon lookup."""

    def __init__(self, borders_dir: str) -> None:
        poly_files = [
            f for f in os.listdir(borders_dir) if f.endswith(".poly")
        ]
        if not poly_files:
            raise ValueError(f"No .poly files found in {borders_dir}")

        self._names: list[str] = []
        self._polys: list[Polygon] = []

        logger.info(f"Loading {len(poly_files)} border polygons...")
        for fn in poly_files:
            rings = _parse_poly(os.path.join(borders_dir, fn))
            if not rings:
                continue
            outer = rings[0]
            holes = rings[1:] if len(rings) > 1 else []
            self._names.append(fn[:-5])
            self._polys.append(Polygon(outer, holes))

        self._strtree = STRtree(self._polys)

    def find(self, lon: float, lat: float) -> str | None:
        """Return the MWM name for the region containing (lon, lat), or None."""
        pt = Point(lon, lat)
        for idx in self._strtree.query(pt):
            if self._polys[idx].contains(pt):
                return self._names[idx]
        return None


def _find_address_geojsons(zf: zipfile.ZipFile) -> list[str]:
    """Return all address geojson paths found in the ZIP.

    OA batch output names files ``<source>-<layer>-<coverage>.geojson``,
    so address files always contain "addresses" in the basename.  Countrywide
    sources without a sub-national directory may also be named
    ``countrywide.geojson`` (no "addresses" component), so we accept either.

    Matches any depth — covers both ``<cc>/countrywide.geojson`` (Canada-style)
    and ``<cc>/<state>/<source>-addresses-<coverage>.geojson`` (US-style) as
    well as any future layouts.
    """
    found = []
    for name in zf.namelist():
        if not name.endswith(".geojson"):
            continue
        basename = name.split("/")[-1]
        if "addresses" in basename or "countrywide" in basename:
            found.append(name)
    if not found:
        raise ValueError(
            "No address geojson files found in ZIP. "
            "Expected files whose basename contains 'addresses' or 'countrywide'."
        )
    return found


def process(
    zip_path: str,
    output_dir: str,
    borders_dir: str,
) -> None:
    os.makedirs(output_dir, exist_ok=True)

    index = _BorderIndex(borders_dir)

    writers: dict[str, io.BufferedWriter] = {}

    def get_writer(mwm_name: str) -> io.BufferedWriter:
        if mwm_name not in writers:
            path = os.path.join(output_dir, mwm_name + _ADDR_EXT)
            writers[mwm_name] = open(path, "wb")
        return writers[mwm_name]

    total = 0
    skipped_incomplete = 0
    skipped_region = 0
    skipped_duplicate = 0

    # Deduplication key: (number, street, unit, lat_3dp, lon_3dp).
    # Rounding to 3 decimal places (~110 m) catches the same address appearing
    # in multiple overlapping OA sources (e.g. us/or/marion and
    # us/or/marion_and_polk) while still preserving distinct unit entries at
    # the same building.  mwm_name is omitted — it is fully determined by
    # the rounded coordinates and adds no dedup precision.
    seen: set[tuple[str, str, str, float, float]] = set()

    try:
        with zipfile.ZipFile(zip_path, "r") as zf:
            geojson_paths = _find_address_geojsons(zf)
            for geojson_path in geojson_paths:
                logger.info(f"Processing {geojson_path}...")
                with zf.open(geojson_path) as raw:
                    for line_bytes in raw:
                        line_bytes = line_bytes.rstrip(b"\n\r")
                        if not line_bytes:
                            continue
                        total += 1

                        try:
                            feat = json.loads(line_bytes)
                        except json.JSONDecodeError:
                            skipped_incomplete += 1
                            continue

                        props = feat.get("properties", {})
                        number   = (props.get("number")   or "").strip()
                        street   = (props.get("street")   or "").strip()
                        unit     = (props.get("unit")     or "").strip()
                        postcode = (props.get("postcode") or "").strip()

                        if not number or not street:
                            skipped_incomplete += 1
                            continue

                        geom = feat.get("geometry") or {}
                        coords = geom.get("coordinates")
                        if not coords or len(coords) < 2:
                            skipped_incomplete += 1
                            continue

                        lon, lat = float(coords[0]), float(coords[1])

                        mwm_name = index.find(lon, lat)
                        if mwm_name is None:
                            skipped_region += 1
                            continue

                        dedup_key = (number, street, unit, round(lat, 3), round(lon, 3))
                        if dedup_key in seen:
                            skipped_duplicate += 1
                            continue
                        seen.add(dedup_key)

                        ipoint = _point_to_int64(lon, lat)

                        buf = io.BytesIO()
                        _write_string(buf, number)
                        _write_string(buf, number)
                        _write_string(buf, street)
                        _write_string(buf, postcode)
                        buf.write(bytes([_INTERPOL_NONE]))
                        _write_varuint(buf, 1)
                        _write_varint(buf, ipoint)

                        get_writer(mwm_name).write(buf.getvalue())

    finally:
        for f in writers.values():
            f.close()

    logger.info(
        f"Processed {total} entries: "
        f"wrote {total - skipped_incomplete - skipped_region - skipped_duplicate}, "
        f"skipped incomplete: {skipped_incomplete}, "
        f"skipped no region: {skipped_region}, "
        f"skipped duplicate: {skipped_duplicate}"
    )
    logger.info(f"Output: {len(writers)} file(s) in {output_dir}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Convert an OpenAddresses collection ZIP to CoMaps .tempaddr files. "
            "Addresses are assigned to MWM regions via point-in-polygon lookup "
            "against the CoMaps .poly border files."
        )
    )
    parser.add_argument(
        "zip_file",
        help="Path to OpenAddresses collection ZIP (e.g. collection-ca.zip)",
    )
    parser.add_argument(
        "output_dir",
        help="Output directory for .tempaddr files (set as ADDRESSES_PATH in the generator)",
    )
    parser.add_argument(
        "--borders-dir",
        required=True,
        help="Path to directory containing CoMaps .poly border files (data/borders/)",
    )
    args = parser.parse_args()
    process(args.zip_file, args.output_dir, args.borders_dir)


if __name__ == "__main__":
    main()
