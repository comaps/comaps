"""Materialize English fallbacks in an ephemeral iOS build checkout.

Locale source files are managed by Weblate. Do not commit the generated changes.
"""

import argparse
import copy
import os
import plistlib
import sys


def read_strings(filepath):
    translations = {}
    with open(filepath, "r", encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            if '" = "' in line:
                translation_parts = line.split('" = "')
                translations[translation_parts[0][1:]] = translation_parts[1][:-2]
            elif len(line) > 6 and line[:3] == "/* " and line[-3:] == " */":
                comment_line = line
                while comment_line in translations:
                    comment_line = comment_line[:2] + "*" + comment_line[2:]
                translations[comment_line] = ""
    return translations


def serialize_strings(translations):
    lines = []
    for key, value in translations.items():
        if key[:2] == "/*" and key[-2:] == "*/":
            lines.append("/* " + key.split(" ", 1)[1])
        else:
            lines.append(f'"{key}" = "{value}";')
    return "\n".join(lines) + "\n"


def merge_missing(source, translation):
    """Recursively add missing source values without replacing translations."""
    changed = False
    for key, source_value in source.items():
        if key not in translation:
            translation[key] = copy.deepcopy(source_value)
            changed = True
            continue

        translation_value = translation[key]
        if isinstance(source_value, dict):
            if not isinstance(translation_value, dict):
                translation[key] = copy.deepcopy(source_value)
                changed = True
            else:
                changed |= merge_missing(source_value, translation_value)
        elif isinstance(translation_value, dict):
            translation[key] = copy.deepcopy(source_value)
            changed = True
    return changed


def update_strings(source_filepath, translation_filepath):
    source = read_strings(source_filepath)
    if os.path.exists(translation_filepath):
        translation = read_strings(translation_filepath)
        merged = {
            key: translation.get(key, value)
            for key, value in source.items()
        }
    else:
        merged = source
    serialized = serialize_strings(merged)
    current = None
    if os.path.exists(translation_filepath):
        with open(translation_filepath, "r", encoding="utf-8") as file:
            current = file.read()
    return serialized, serialized != current


def update_stringsdict(source_filepath, translation_filepath):
    with open(source_filepath, "rb") as file:
        source = plistlib.load(file)
    with open(translation_filepath, "rb") as file:
        translation = plistlib.load(file)
    changed = merge_missing(source, translation)
    return plistlib.dumps(
        translation, fmt=plistlib.FMT_XML, sort_keys=False
    ), changed


def regenerate(base_directory, file_type="all", check=False):
    english_directory = os.path.join(base_directory, "en.lproj")
    language_directories = sorted(
        directory
        for directory in os.listdir(base_directory)
        if directory.endswith(".lproj") and directory != "en.lproj"
    )
    changed_files = []

    if file_type in ("all", "strings"):
        for filename in sorted(os.listdir(english_directory)):
            if not filename.endswith(".strings"):
                continue
            source_filepath = os.path.join(english_directory, filename)
            for language_directory in language_directories:
                filepath = os.path.join(base_directory, language_directory, filename)
                serialized, changed = update_strings(source_filepath, filepath)
                if not changed:
                    continue
                changed_files.append(filepath)
                if not check:
                    with open(filepath, "w", encoding="utf-8") as file:
                        file.write(serialized)

    if file_type in ("all", "stringsdict"):
        for filename in sorted(os.listdir(english_directory)):
            if not filename.endswith(".stringsdict"):
                continue
            source_filepath = os.path.join(english_directory, filename)
            for language_directory in language_directories:
                filepath = os.path.join(base_directory, language_directory, filename)
                # Only Xcode/Weblate should create a plural resource for a locale.
                if not os.path.exists(filepath):
                    continue
                serialized, changed = update_stringsdict(source_filepath, filepath)
                if not changed:
                    continue
                changed_files.append(filepath)
                if not check:
                    with open(filepath, "wb") as file:
                        file.write(serialized)

    return changed_files


def parse_args():
    parser = argparse.ArgumentParser(
        description="Fill missing iOS translations with their English fallback."
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Report files that need regeneration without changing them.",
    )
    parser.add_argument(
        "--file-type",
        choices=("all", "strings", "stringsdict"),
        default="all",
        help="Limit regeneration to one localization file type.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    base_directory = os.path.abspath(
        os.path.join(
            os.path.realpath(__file__),
            "..",
            "..",
            "..",
            "iphone",
            "Maps",
            "LocalizedStrings",
        )
    )
    changed_files = regenerate(base_directory, args.file_type, args.check)
    if args.check and changed_files:
        print("Localization files need regeneration:")
        for filepath in changed_files:
            print(os.path.relpath(filepath, base_directory))
        return 1
    if not args.check:
        print(f"Updated {len(changed_files)} localization files.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
