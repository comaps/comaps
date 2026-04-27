import re
import json
from datetime import datetime, timezone
from pathlib import Path

SCRIPT_DIR: Path = Path(__file__).parent.resolve()
MAPCSS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/include/Icons.mapcss"
BASECSS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/include/Basemap.mapcss"

LIGHTSTYLE_COLORS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/light/colors.mapcss"
DARKSTYLE_COLORS_FILE: Path = SCRIPT_DIR / "../../data/styles/default/dark/colors.mapcss"


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
                if key.startswith("!"):
                    continue  # skip negated keys
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


def main() -> None:
    '''
    Extract information from css files to create
    taginfo.json to list CoMaps as a project
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

    tags = list = mapcss_tags + basecss_tags

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
