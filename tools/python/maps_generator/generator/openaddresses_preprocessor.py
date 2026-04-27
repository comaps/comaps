#!/usr/bin/env python3
"""Converts an OpenAddresses ZIP to CoMaps .tempaddr address files."""

import argparse
import io
import json
import math
import os
import sys
import zipfile

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
    ux = _double_to_uint32(lon,           _MIN_X, _MAX_X)
    uy = _double_to_uint32(_lat_to_y(lat), _MIN_Y, _MAX_Y)
    return int(_perfect_shuffle((uy << 32) | ux))


def _collect_mwm_names(countries_txt: str) -> list[str]:
    with open(countries_txt, encoding="utf-8") as f:
        root = json.load(f)
    names: list[str] = []
    stack = [root]
    while stack:
        node = stack.pop()
        if isinstance(node, dict):
            n = node.get("id", "")
            if n:
                names.append(n)
            v = node.get("g", node.get("v", []))
            if isinstance(v, list):
                stack.extend(v)
        elif isinstance(node, list):
            stack.extend(node)
    return names


def _find_countrywide_geojson(zf: zipfile.ZipFile) -> str:
    for name in zf.namelist():
        parts = name.split("/")
        if len(parts) == 2 and "countrywide" in parts[1] and parts[1].endswith(".geojson"):
            return name
    raise ValueError(
        "No countrywide geojson found in ZIP (expected <cc>/countrywide*.geojson). "
        "Use --mapping to specify 'geojson_path' and 'region_to_mwm_prefix' manually."
    )


def _build_auto_mapping(geojson_path: str) -> dict[str, str]:
    try:
        import pycountry
    except ImportError as exc:
        raise ImportError(
            "pycountry is required for automatic region mapping. "
            "Install it (`pip install pycountry`) or use --mapping."
        ) from exc
    country_code = geojson_path.split("/")[0].upper()
    country = pycountry.countries.get(alpha_2=country_code)
    if country is None:
        raise ValueError(
            f"Cannot determine country from geojson path '{geojson_path}'. Use --mapping."
        )
    subdivisions = pycountry.subdivisions.get(country_code=country_code)
    if not subdivisions:
        raise ValueError(
            f"No ISO 3166-2 subdivisions found for '{country_code}'. Use --mapping."
        )
    return {
        s.code.split("-", 1)[1]: f"{country.name}_{s.name}"
        for s in subdivisions
    }


def _build_region_mwm_map(
    region_to_mwm_prefix: dict[str, str],
    countries_txt: str | None,
) -> dict[str, list[str]]:
    if countries_txt is not None:
        all_names = _collect_mwm_names(countries_txt)
    else:
        all_names = None

    result: dict[str, list[str]] = {}
    for region, prefix in region_to_mwm_prefix.items():
        if all_names is None:
            result[region] = [prefix]
        else:
            matches = [
                n for n in all_names
                if n == prefix or n.startswith(prefix + "_")
            ]
            result[region] = matches if matches else [prefix]
    return result


def process(
    zip_path: str,
    output_dir: str,
    mapping: dict | None = None,
    countries_txt: str | None = None,
) -> None:
    os.makedirs(output_dir, exist_ok=True)

    writers: dict[str, io.BufferedWriter] = {}

    def get_writers(mwm_names: list[str]) -> list[io.BufferedWriter]:
        result = []
        for name in mwm_names:
            if name not in writers:
                path = os.path.join(output_dir, name + _ADDR_EXT)
                writers[name] = open(path, "wb")
            result.append(writers[name])
        return result

    total = 0
    skipped_incomplete = 0
    skipped_region = 0

    try:
        with zipfile.ZipFile(zip_path, "r") as zf:
            if mapping is not None:
                geojson_path: str = mapping["geojson_path"]
                region_to_mwm_prefix: dict[str, str] = mapping["region_to_mwm_prefix"]
            else:
                geojson_path = _find_countrywide_geojson(zf)
                region_to_mwm_prefix = _build_auto_mapping(geojson_path)

            region_to_mwms = _build_region_mwm_map(region_to_mwm_prefix, countries_txt)

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
                    number   = props.get("number",   "").strip()
                    street   = props.get("street",   "").strip()
                    postcode = props.get("postcode", "").strip()
                    region   = props.get("region",   "").strip().upper()

                    if not number or not street:
                        skipped_incomplete += 1
                        continue

                    geom = feat.get("geometry") or {}
                    coords = geom.get("coordinates")
                    if not coords or len(coords) < 2:
                        skipped_incomplete += 1
                        continue

                    mwm_names = region_to_mwms.get(region)
                    if mwm_names is None:
                        skipped_region += 1
                        continue

                    lon, lat = float(coords[0]), float(coords[1])
                    ipoint = _point_to_int64(lon, lat)

                    record = bytearray()
                    buf = io.BytesIO()
                    _write_string(buf, number)
                    _write_string(buf, number)
                    _write_string(buf, street)
                    _write_string(buf, postcode)
                    buf.write(bytes([_INTERPOL_NONE]))
                    _write_varuint(buf, 1)
                    _write_varint(buf, ipoint)
                    record = buf.getvalue()

                    for f in get_writers(mwm_names):
                        f.write(record)

    finally:
        for f in writers.values():
            f.close()

    print(
        f"Processed {total} entries: "
        f"wrote {total - skipped_incomplete - skipped_region}, "
        f"skipped incomplete: {skipped_incomplete}, "
        f"skipped unknown region: {skipped_region}",
        file=sys.stderr,
    )
    print(f"Output: {len(writers)} file(s) in {output_dir}", file=sys.stderr)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert an OpenAddresses ZIP to CoMaps .tempaddr files"
    )
    parser.add_argument("zip_file", help="Path to OpenAddresses collection ZIP (e.g. collection-ca.zip)")
    parser.add_argument("output_dir", help="Output directory for .tempaddr files (set as ADDRESSES_PATH)")
    parser.add_argument(
        "--mapping",
        help=(
            "Path to a JSON mapping file with 'geojson_path' and 'region_to_mwm_prefix' keys. "
            "If omitted, the countrywide geojson is auto-detected from the ZIP and regions are "
            "derived from ISO 3166-2 subdivisions via pycountry."
        ),
    )
    parser.add_argument("--countries-txt", help="Path to countries.txt to generate per-sub-region files")
    args = parser.parse_args()
    mapping = None
    if args.mapping:
        with open(args.mapping, encoding="utf-8") as f:
            mapping = json.load(f)
    process(args.zip_file, args.output_dir, mapping, args.countries_txt)


if __name__ == "__main__":
    main()
