#!/usr/bin/env python3
"""Extract address records from an OpenAddresses ZIP and emit them as TSV.

Each address feature found in the GeoJSON files inside the ZIP is written as a
tab-separated line to stdout::

    lat  lon  house_number  street  postcode  editable  source_name  license_url  license_name

Region assignment (point-in-polygon against .poly border files) and .tempaddr
binary encoding are handled by the C++ ``oa_processor_tool``.

Usage:
    python3 openaddresses_preprocessor.py <zip_file> [--oa-sources-dir <path>]
"""

import argparse
import json
import logging
import os
import re
import sys
import urllib.error
import urllib.request
import zipfile

logger = logging.getLogger("maps_generator")


def _find_address_geojsons(zf: zipfile.ZipFile) -> list[str]:
    """Return all address geojson paths found in the ZIP."""
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


_LAYER_SUFFIX_RE = re.compile(
    r"-(?:addresses|buildings|parcels|centerlines)-[^/]+\.geojson$",
    re.IGNORECASE,
)

_INCOMPATIBLE_LICENSE_SUBSTRINGS: tuple[str, ...] = (
    "non-commercial", "noncommercial",
    "no derivatives", "no-derivatives", "noderivatives",
    "-nc-", "-nc/", "-nd-", "-nd/",
    "-sa-", "-sa/",
)


def _source_key_from_geojson_path(geojson_path: str) -> str:
    key = _LAYER_SUFFIX_RE.sub("", geojson_path)
    if key == geojson_path:
        key = geojson_path.removesuffix(".geojson")
    return key


def _license_is_odbl_compatible(license_info) -> bool:
    if isinstance(license_info, str):
        url = license_info.lower()
        text = ""
    else:
        url = (license_info.get("url") or "").lower()
        text = (license_info.get("text") or "").lower()
    for term in _INCOMPATIBLE_LICENSE_SUBSTRINGS:
        if term in url or term in text:
            return False
    return True


_OA_GITHUB_RAW = "https://raw.githubusercontent.com/openaddresses/openaddresses/master"
_source_editable_cache: dict[str, bool] = {}


def _load_source_json(source_key: str, oa_sources_dir: str | None = None) -> dict | None:
    if oa_sources_dir:
        local_path = os.path.join(oa_sources_dir, "sources", source_key + ".json")
        try:
            with open(local_path, "rb") as f:
                return json.loads(f.read().decode("utf-8"))
        except FileNotFoundError:
            logger.warning(f"Source JSON not found locally for {source_key!r} ({local_path})")
            return None
        except Exception as exc:
            logger.warning(f"Cannot read local source JSON for {source_key!r} ({local_path}): {exc}")
            return None

    keys_to_try = [source_key]
    normalised = source_key.replace("-", "_")
    if normalised != source_key:
        keys_to_try.append(normalised)

    for key in keys_to_try:
        url = f"{_OA_GITHUB_RAW}/sources/{key}.json"
        try:
            with urllib.request.urlopen(url, timeout=10) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as exc:
            if exc.code == 404:
                continue
            logger.warning(
                f"Cannot fetch source JSON for {source_key!r} ({url}): {exc};"
                " defaulting to ODbL-compatible"
            )
            return None
        except Exception as exc:
            logger.warning(
                f"Cannot fetch source JSON for {source_key!r} ({url}): {exc};"
                " defaulting to ODbL-compatible"
            )
            return None

    logger.warning(
        f"Source JSON not found for {source_key!r} on GitHub (HTTP 404);"
        " defaulting to ODbL-compatible"
    )
    return None


def _get_source_attribution(source_key: str, oa_sources_dir: str | None = None) -> tuple[str, str, str]:
    source = _load_source_json(source_key, oa_sources_dir)
    if source is None:
        return ("", "", "")
    address_layers = source.get("layers", {}).get("addresses", [])
    for layer_entry in address_layers:
        src = (layer_entry.get("attribution")
               or layer_entry.get("website")
               or "")
        license_info = layer_entry.get("license")
        if isinstance(license_info, str):
            return (src, license_info, "")
        if isinstance(license_info, dict):
            if not src:
                src = license_info.get("attribution name", "")
            url = (license_info.get("url") or "").strip()
            text = (license_info.get("text") or "").strip()
            if url or text:
                return (src, url, text)
        if src:
            return (src, "", "")
    return ("", "", "")


def _is_odbl_compatible_source(source_key: str, oa_sources_dir: str | None = None) -> bool:
    if source_key in _source_editable_cache:
        return _source_editable_cache[source_key]
    source = _load_source_json(source_key, oa_sources_dir)
    if source is None:
        _source_editable_cache[source_key] = True
        return True
    address_layers = source.get("layers", {}).get("addresses", [])
    for layer_entry in address_layers:
        license_info = layer_entry.get("license")
        if not license_info:
            continue
        if not _license_is_odbl_compatible(license_info):
            logger.info(f"Source {source_key!r} has incompatible license clause, marking as non-editable.")
            _source_editable_cache[source_key] = False
            return False
    _source_editable_cache[source_key] = True
    return True


def process(
    zip_path: str,
    out_file=sys.stdout,
    oa_sources_dir: str | None = None,
) -> None:
    total = 0
    skipped_incomplete = 0
    skipped_duplicate = 0
    seen_addr: set[tuple[str, str, str]] = set()

    with zipfile.ZipFile(zip_path, "r") as zf:
        geojson_paths = _find_address_geojsons(zf)
        geojson_paths = sorted(
            geojson_paths,
            key=lambda p: (
                0 if _is_odbl_compatible_source(_source_key_from_geojson_path(p), oa_sources_dir)
                else 1
            ),
        )
        for geojson_path in geojson_paths:
            source_key = _source_key_from_geojson_path(geojson_path)
            editable = _is_odbl_compatible_source(source_key, oa_sources_dir)
            src_name, lic_url, lic_name = _get_source_attribution(source_key, oa_sources_dir)
            logger.info(f"Processing {geojson_path} (editable={editable})...")
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
                    lat, lon = float(coords[1]), float(coords[0])
                    addr_key = (number, street, unit)
                    if addr_key in seen_addr:
                        skipped_duplicate += 1
                        continue
                    seen_addr.add(addr_key)
                    editable_flag = "1" if editable else "0"
                    out_file.write("\t".join([
                        str(lat), str(lon), number, street, postcode,
                        editable_flag, src_name, lic_url, lic_name,
                    ]) + "\n")

    out_file.flush()
    logger.info(
        f"Processed {total} entries: "
        f"wrote {total - skipped_incomplete - skipped_duplicate}, "
        f"skipped incomplete: {skipped_incomplete}, "
        f"skipped duplicate: {skipped_duplicate}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Extract address records from an OpenAddresses collection ZIP and emit "
            "them as TSV to stdout.  Region assignment and .tempaddr encoding are "
            "handled by the C++ oa_processor_tool."
        )
    )
    parser.add_argument("zip_file", help="Path to OpenAddresses collection ZIP")
    parser.add_argument(
        "--oa-sources-dir",
        default=None,
        help="Path to a local clone of github.com/openaddresses/openaddresses.",
    )
    args = parser.parse_args()
    process(args.zip_file, oa_sources_dir=args.oa_sources_dir)


if __name__ == "__main__":
    main()
