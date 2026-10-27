import re
import json
import xml.etree.ElementTree as ET
from collections.abc import Callable
from datetime import datetime, timezone
from pathlib import Path

SCRIPT_DIR: Path = Path(__file__).parent.resolve()
MAPCSS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/include/Icons.mapcss"
BASECSS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/include/Basemap.mapcss"

LIGHTSTYLE_COLORS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/light/colors.mapcss"
DARKSTYLE_COLORS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/dark/colors.mapcss"

MAPPING_FILE: Path = SCRIPT_DIR / "../../data/mapcss-mapping.csv"
REPLACED_TAGS_FILE: Path = SCRIPT_DIR / "../../data/replaced_tags.txt"
EXTENSION_FILE: Path = SCRIPT_DIR / "../../data/taginfo_extension.json"
ROAD_ACCESS_FILE: Path = SCRIPT_DIR / "../../generator/road_access_generator.cpp"
ROAD_PENALTY_FILE: Path = SCRIPT_DIR / "../../generator/road_penalty_generator.cpp"
OSM2TYPE_FILE: Path = SCRIPT_DIR / "../../generator/osm2type.cpp"
RESTRICTION_FILE: Path = SCRIPT_DIR / "../../generator/restriction_writer.cpp"
METADATA_FILE: Path = SCRIPT_DIR / "../../libs/indexer/feature_meta.cpp"
LANGUAGES_FILE: Path = SCRIPT_DIR / "../../libs/i18n/localisation.hpp"
EDITOR_CONFIG_FILE: Path = SCRIPT_DIR / "../../data/editor.config"
VEHICLE_MODEL_FILE: Path = SCRIPT_DIR / "../../libs/routing_common/vehicle_model.hpp"
ROUTING_MODEL_FILES: list[Path] = [
    SCRIPT_DIR / "../../libs/routing_common/car_model.cpp",
    SCRIPT_DIR / "../../libs/routing_common/bicycle_model.cpp",
    SCRIPT_DIR / "../../libs/routing_common/pedestrian_model.cpp",
]

TYPE_NAMES_FILE: Path = SCRIPT_DIR / "../../iphone/Maps/LocalizedStrings/en.lproj/LocalizableTypes.strings"
INTERNAL_KEYS: set[str] = {"hwtag", "psurface", "_path_grade", "city", "dont", "isoline", "mapswithme", "complex_entry"}
WILDCARD_VALUES: set[str] = {"*", "!", "~", "!r", "~r"}

ROUTING_DESCRIPTION: str = "Taken into account for routing"
DETAILS_DESCRIPTION: str = "Shown as place/POI details"
ADDRESS_DESCRIPTION: str = "Used for addresses and address search"
BUILDINGS_3D_DESCRIPTION: str = "Used to draw 3D buildings"
EDITOR_DESCRIPTION: str = "Editable in the app's map editor"
NAME_DESCRIPTION: str = "Used as the name on the map, in place/POI details and in search"
SEARCH_DESCRIPTION: str = "Used for search"
NAME_KEYS: dict[str, tuple[str, str]] = {
    "default": ("name", NAME_DESCRIPTION),
    "int_name": ("int_name", NAME_DESCRIPTION),
    "alt_name": ("alt_name", SEARCH_DESCRIPTION),
    "old_name": ("old_name", SEARCH_DESCRIPTION),
}
METADATA_DESCRIPTIONS: dict[str, str] = {
    "FMD_HEIGHT": BUILDINGS_3D_DESCRIPTION,
    "FMD_MIN_HEIGHT": BUILDINGS_3D_DESCRIPTION,
    "FMD_BUILDING_LEVELS": BUILDINGS_3D_DESCRIPTION,
    "FMD_BUILDING_MIN_LEVEL": BUILDINGS_3D_DESCRIPTION,
    "FMD_DESTINATION": ROUTING_DESCRIPTION,
    "FMD_DESTINATION_REF": ROUTING_DESCRIPTION,
    "FMD_JUNCTION_REF": ROUTING_DESCRIPTION,
    "FMD_TURN_LANES": ROUTING_DESCRIPTION,
    "FMD_TURN_LANES_FORWARD": ROUTING_DESCRIPTION,
    "FMD_TURN_LANES_BACKWARD": ROUTING_DESCRIPTION,
    "FMD_DURATION": ROUTING_DESCRIPTION,
    "FMD_POSTCODE": ADDRESS_DESCRIPTION,
    "FMD_FAX_NUMBER": "",
    "FMD_DENOMINATION": "",
}
RULE_BLOCK_DESCRIPTIONS: dict[str, str] = {
    "oneway": ROUTING_DESCRIPTION,
    "ferry": ROUTING_DESCRIPTION,
    "traffic_signals:direction": ROUTING_DESCRIPTION,
    "network": "Used to choose the metro icon",
}

TAGINFO_FILE: Path = SCRIPT_DIR / "../../data/taginfo.json"

BASE_ICON_URL: str = "https://codeberg.org/comaps/comaps/raw/branch/main/data/styles/default/light/symbols/"

PROJECT_INFO: dict[str, str] = {
    "name": "CoMaps",
    "description": "CoMaps is a community-focused privacy navigation iOS & Android app for travelers - drivers, hikers, and cyclists.",
    "project_url": "https://comaps.app",
    "doc_url": "https://codeberg.org/comaps/comaps/",
    "icon_url": "https://codeberg.org/comaps/comaps/media/branch/main/docs/badges/logo.svg",
    "contact_name": "CoMaps",
    "contact_email": "hello@comaps.app"
}

def parse_colors(text: str) -> dict[str, str]:
    '''
    Read color mapcss file for style to extract
    color names used on map per style
    '''
    colors: dict[str, str] = {}
    color_blocks: list[tuple[str, str]] = re.findall(r"^(@.*): (#.*);", text, re.MULTILINE)
    for item, color in color_blocks:
        colors[item] = color
    return colors


def parse_mapcss(text: str, colors: dict[str, dict] = {}) -> list[dict[str, any]]:
    '''
    Parse CSS file, either Icon.mapcss or Basemap.mapcss
    If base CSS, can also give color definitions
    '''
    tags: dict[tuple[str, str | None, str], dict[str, any]] = {}
    # Split blocks into: selector { props }
    blocks: list[tuple[str, str]] = re.findall(r"([^\{]+)\{([^\}]*)\}", text, re.MULTILINE)

    for selector, props in blocks:
        # Extract icon filename from props
        icon_re: re.Pattern = re.compile(r"icon-image:\s*([^;]+);")
        icon_match: re.Match | None = icon_re.search(props)
        icon_url: str | None = None
        if icon_match:
            icon_file: str = icon_match.group(1).strip()
            if icon_file and icon_file.lower() not in ["none", "zero-icon.svg"]:
                icon_url = BASE_ICON_URL + icon_file

        # Split the selector into lines
        lines: list[str] = [line.strip() for line in selector.split("\n") if line.strip()]
        for line in lines:
            # Find anything inside square brackets
            square_brackets_re: re.Pattern = re.compile(r"\[(.*?)\]")
            square_brackets: list[str] = square_brackets_re.findall(line)
            if not square_brackets:
                continue

            # Find key=value pairs
            pairs: list[tuple[str, str | None]] = []
            for sqb in square_brackets:
                key, sep, value = sqb.partition("=")
                key = key.strip()
                if key.startswith("!") or key.endswith("!"):
                    continue  # skip negated keys
                key = key.rstrip("?")
                value = value.strip() if sep else None
                pairs.append((key, value))

            # Hardcode: convert value "not" to "no"
            pairs = [(k, "no" if v == "not" else v) for k, v in pairs]

            if not pairs:
                continue  # skip if no valid pairs

            # Build shared description from all pairs
            desc: str = " + ".join(f"{k}={v if v is not None else '*'}" for k, v in pairs)

            emit = True # generally do emit description, but…

            # if we loaded the color keys, need to check if 
            # there are colors to append, and if  ot we do not 
            # want to emit an empty description:
            if colors:
                # Default to not emit in that case
                emit = False
                color_desc: str = ""
                color_props = re.findall(r"(\w*-?color:)*? (@\w*);*?", props)
                for ctype, color in color_props:
                    for style, ctable in colors.items():
                        if color in ctable.keys():
                            color_desc += "{ctype} ({style}): {translated_color}, ".format(
                                    ctype=ctype, style=style, translated_color=ctable[color])
                            emit = True # found at least one color, we can emit tag
                desc += " | {}".format(color_desc.strip(', '))

            # Emit a tag per pair
            for key, value in pairs:
                if key != "isoline" and emit: # ignore empty color keys and isolines (not from OSM)
                    tag_id: tuple[str, str | None, str] = (key, value, desc)
                    if tag_id not in tags:
                        tag: dict[str, any] = {
                            "description": desc,
                            "key": key,
                        }
                        if value is not None:
                            tag["value"] = value
                        if icon_url:
                            tag["icon_url"] = icon_url
                        tags[tag_id] = tag
                    else:
                        if icon_url:
                            tags[tag_id]["icon_url"] = icon_url

    # Sort by description, then key, then value
    return sorted(tags.values(), key=lambda x: (x["description"], x["key"], x.get("value", "")))


def parse_type_names(text: str) -> dict[str, str]:
    return dict(re.findall(r'^"type\.([^"]+)" = "([^"]*)";', text, re.MULTILINE))


def parse_mapping_csv(text: str, type_names: dict[str, str]) -> list[tuple[str, str | None, str]]:
    found: list[tuple[str, str | None, str]] = []
    for line in text.splitlines():
        if not line or line.startswith("#"):
            continue
        fields: list[str] = line.split(";")
        desc: str = type_names.get(fields[0].replace("|", "."), "")
        if fields[0].startswith("cuisine|"):
            desc = DETAILS_DESCRIPTION
        if len(fields) == 3 and not fields[2]:
            key, value = fields[0].split("|")
            found.append((key, value, desc))
        elif len(fields) == 7 and fields[2] != "x":
            for selector in fields[1].split(","):
                parts: list[str] = re.findall(r"\[([^\]]*)\]", selector)
                for kv in parts:
                    if kv.startswith("!"):
                        continue
                    key, sep, value = kv.partition("=")
                    tag_value: str | None = ("no" if value == "not" else value) if sep else None
                    found.append((key.rstrip("?"), tag_value, desc if len(parts) == 1 else ""))
    described = [t for t in found if t[2]]
    undescribed = [t for t in found if not t[2]]
    return described + undescribed


def parse_replaced_tags(text: str) -> list[tuple[str, str | None, str]]:
    found: list[tuple[str, str | None, str]] = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        key, _, rest = line.partition("=")
        value, _, replacement = rest.partition(":")
        targets: str = ", ".join(t.strip() for t in replacement.split("|")[0].split(","))
        found.append((key.strip(), value.strip(), f"Treated as {targets}"))
    return found


def scrape_tag_calls(text: str) -> list[tuple[str, str | None, str]]:
    tags: list[tuple[str, str]] = re.findall(r'OsmElement::Tag\("([^"]+)",\s*"([^"]*)"\)', text)
    return [(k, v, ROUTING_DESCRIPTION) for k, v in tags]


def scrape_conditional_keys(text: str) -> list[tuple[str, str | None, str]]:
    return [(k, None, ROUTING_DESCRIPTION) for k in re.findall(r'"([a-z_]+:conditional)"', text)]


def scrape_tag_rules(text: str) -> list[tuple[str, str | None, str]]:
    found: list[tuple[str, str | None, str]] = []
    for block in text.split("ApplyRules")[1:]:
        rules: list[tuple[str, str]] = re.findall(r'\{\s*"([^"]+)",\s*"([^"]*)",\s*\[', block.split("});")[0])
        desc: str = next((d for marker, d in RULE_BLOCK_DESCRIPTIONS.items() if marker in {k for k, _ in rules}), "")
        for k, v in rules:
            address: bool = k.startswith(("addr:", "contact:")) or k == "postal_code"
            found.append((k, None if v in WILDCARD_VALUES else v, ADDRESS_DESCRIPTION if address else desc))
    return found


def scrape_restrictions(text: str) -> list[tuple[str, str | None, str]]:
    return [("restriction", v, ROUTING_DESCRIPTION) for v in re.findall(r'\{"([a-z_]+)",\s*Restriction::Type::', text)]


def scrape_metadata_keys(text: str) -> list[tuple[str, str | None, str]]:
    function: re.Match | None = re.search(r"bool Metadata::TypeFromString\(.*?\n}", text, re.DOTALL)
    if not function:
        return []
    found: list[tuple[str, str | None, str]] = []
    branches: list[str] = re.split(r"outType = Metadata::(?:EType::)?(FMD_\w+);", function.group(0))
    for condition, metadata_type in zip(branches[0::2], branches[1::2]):
        keys: list[str] = re.findall(r'k == "([^"]+)"', condition)
        keys += re.findall(r'(?:starts_with\(|GetLanguageIndexForKey\(k, )"([^"]+)"\)', condition)
        desc: str = METADATA_DESCRIPTIONS.get(metadata_type, DETAILS_DESCRIPTION)
        found.extend((k, None, desc) for k in keys if not k.endswith(":"))
    return found


def scrape_road_types(text: str, mapping: str, models: list[str]) -> list[tuple[str, str | None, str]]:
    if "enum class HighwayType" not in text:
        return []
    enum: str = text.split("enum class HighwayType")[1].split("};")[0]
    indices: dict[str, int] = {name: int(index) for name, index in re.findall(r"(\w+) = (\d+),", enum)}
    used: set[str] = {name for model in models for name in re.findall(r"HighwayType::(\w+)", model)}
    selectors: dict[int, list[tuple[str, str | None]]] = {}
    for line in mapping.splitlines():
        if line.startswith("#"):
            continue
        fields: list[str] = line.split(";")
        if len(fields) == 3:
            selectors[int(fields[1])] = [tuple(fields[0].split("|"))]
        elif len(fields) == 7:
            parts: list[str] = re.findall(r"\[([^\]]*)\]", fields[1].split(",")[0])
            pairs: list[tuple[str, str, str]] = [p.partition("=") for p in parts if not p.startswith("!")]
            selectors[int(fields[5])] = [(k.rstrip("?"), v or None) for k, _, v in pairs]
    return [(k, v, ROUTING_DESCRIPTION) for name in used for k, v in selectors[indices[name] + 1]]


def parse_editor_config(text: str) -> list[tuple[str, str | None, str]]:
    root: ET.Element = ET.fromstring(text)
    included: set[str] = {e.get("field") for e in root.iter("include")}
    included |= {e.get("name") for e in root.iter("field_ref")}
    found: list[tuple[str, str | None, str]] = []
    for field in root.iter("field"):
        tag: ET.Element | None = field.find("tag")
        if tag is not None and field.get("editable") != "no" and field.get("name") in included:
            found.append((tag.get("k"), None, EDITOR_DESCRIPTION))
    return found


def scrape_name_languages(text: str) -> list[tuple[str, str | None, str]]:
    table: re.Match | None = re.search(r"kLanguages = \{(.*?)\};", text, re.DOTALL)
    if not table:
        return []
    found: list[tuple[str, str | None, str]] = []
    for code in re.findall(r'^\s*\{+"([^"]+)", "', table.group(1), re.MULTILINE):
        key, desc = NAME_KEYS.get(code, (f"name:{code}", NAME_DESCRIPTION))
        found.append((key, None, desc))
    return found


def merge_tags(tags: list[dict[str, any]], extra: list[tuple[str, str | None, str]], by_description: bool) -> None:
    seen: set[tuple] = {(t["key"], t.get("value"), t.get("description", "")) for t in tags}
    seen_tags: set[tuple[str, str | None]] = {(t["key"], t.get("value")) for t in tags}
    added: list[dict[str, any]] = []
    for key, value, desc in extra:
        if key in INTERNAL_KEYS or (key, value, desc) in seen:
            continue
        if (key, value) in seen_tags and not (by_description and desc):
            continue
        seen.add((key, value, desc))
        seen_tags.add((key, value))
        tag: dict[str, any] = {"description": desc, "key": key} if desc else {"key": key}
        if value is not None:
            tag["value"] = value
        added.append(tag)
    tags.extend(sorted(added, key=lambda x: (x["key"], x.get("value", ""))))


def main() -> None:
    '''
    Creates data/taginfo.json by extracting information from style files, data files and generator source code
    (see constants at the top of the file) and merging with data/taginfo_extension.json, a hand-maintained list of
    tags not found automatically.
    '''

    # extract color definitons to put into taginfo descriptions

    with open(LIGHTSTYLE_COLORS_FILE, "r", encoding="utf-8") as f:
        lightcolor_css: str = f.read()

    lightcolors:  dict[str, str] = parse_colors(lightcolor_css)

    with open(DARKSTYLE_COLORS_FILE, "r", encoding="utf-8") as f:
        darkcolor_css: str = f.read()

    darkcolors:  dict[str, str] = parse_colors(darkcolor_css)

    colors: dict[str, dict] = {'light': lightcolors, 'dark': darkcolors}

    # read mapcss file for getting POIs with icons
    with open(MAPCSS_FILE, "r", encoding="utf-8") as f:
        mapcss: str = f.read()

    mapcss_tags: list[dict[str, any]] = parse_mapcss(mapcss)

    # read basecss file to get tags w/o POI icons attached
    with open(BASECSS_FILE, "r", encoding="utf-8") as f:
        basecss: str = f.read()

    basecss_tags:  list[dict[str, any]] = parse_mapcss(basecss, colors)

    tags: list[dict[str, any]] = mapcss_tags + basecss_tags

    type_names: dict[str, str] = parse_type_names(TYPE_NAMES_FILE.read_text(encoding="utf-8"))
    mapping: str = MAPPING_FILE.read_text(encoding="utf-8")
    models: list[str] = [path.read_text(encoding="utf-8") for path in ROUTING_MODEL_FILES]

    sources: list[tuple[Path, Callable[[str], list[tuple[str, str | None, str]]]]] = [
        (MAPPING_FILE, lambda text: parse_mapping_csv(text, type_names)),
        (REPLACED_TAGS_FILE, parse_replaced_tags),
        (EDITOR_CONFIG_FILE, parse_editor_config),
        (METADATA_FILE, scrape_metadata_keys),
        (LANGUAGES_FILE, scrape_name_languages),
        (ROAD_ACCESS_FILE, scrape_tag_calls),
        (ROAD_ACCESS_FILE, scrape_conditional_keys),
        (ROAD_PENALTY_FILE, scrape_tag_calls),
        (RESTRICTION_FILE, scrape_restrictions),
        (OSM2TYPE_FILE, scrape_tag_rules),
        (VEHICLE_MODEL_FILE, lambda text: scrape_road_types(text, mapping, models)),
    ]
    for path, parse in sources:
        found: list[tuple[str, str | None, str]] = parse(path.read_text(encoding="utf-8"))
        if not found:
            # if any source yields no tags, stop the script
            # this should catch cases where the C++ has changed and a regex needs updating
            raise SystemExit(f"❌ No tags found in {path}")
        merge_tags(tags, found, path != MAPPING_FILE)
    with open(EXTENSION_FILE, "r", encoding="utf-8") as f:
        merge_tags(tags, [(t["key"], t.get("value"), t.get("description", "")) for t in json.load(f)], True)
    described: set[tuple[str, str | None]] = {(t["key"], t.get("value")) for t in tags if t.get("description")}
    tags = [t for t in tags if t.get("description") or (t["key"], t.get("value")) not in described]

    data: dict[str, any] = {
        "data_format": 1,
        "data_url": "https://codeberg.org/comaps/comaps/raw/branch/main/data/taginfo.json",
        "data_updated": datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ"),
        "project": PROJECT_INFO,
        "tags": tags
    }

    with open(TAGINFO_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4, ensure_ascii=False)

    print(f"✅ JSON saved to {TAGINFO_FILE}")


if __name__ == "__main__":
    main()
