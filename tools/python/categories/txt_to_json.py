#!/usr/bin/env python3
import os
import json
import sys

LANGUAGES = (
    'af', 'ar', 'be', 'bg', 'ca', 'cs', 'da', 'de', 'el', 'en', 'en-AU',
    'en-GB', 'en-US', 'es', 'es-MX', 'et', 'eu', 'fa', 'fi', 'fr', 'fr-CA',
    'he', 'hi', 'hu', 'id', 'it', 'ja', 'ko', 'lt', 'lv', 'mr', 'nb', 'nl',
    'pl', 'pt', 'pt-BR', 'ro', 'ru', 'sk', 'sr', 'sv', 'sw', 'th', 'tr', 'uk',
    'vi', 'zh-Hans', 'zh-Hant'
)

# TODO: respect the order of key/values in the JSON when converting back and forth

def parse_translations(input_file):
    """
    Parses a translation file and generates a JSON file per language.
    """
    # Read the input file line by line
    with open(input_file, 'r', encoding='utf-8') as f:
        lines = [line.rstrip('\n') for line in f]

    # Split the file into blocks separated by empty lines
    blocks = []
    current_block = []
    for line in lines:
        stripped_line = line.strip()
        if stripped_line.startswith('#'):
            continue
        if not stripped_line:
            if current_block:
                blocks.append(current_block)
                current_block = []
        else:
            current_block.append(line)
    if current_block:
        blocks.append(current_block)

    # Initialize dictionaries for each language
    lang_data = {lang: {} for lang in LANGUAGES}

    # Process each block
    for block in blocks:
        key_line = block[0]
        has_translation = False
        for line in block[1:]:
            if ':' not in line:
                print(f"Skipping invalid line: {line}")
                continue
            lang, translation = line.split(':', 1)
            lang = lang.strip()
            translation = translation.strip()
            if lang in LANGUAGES:
                lang_data[lang][key_line] = translation
                has_translation = True
            else:
                print(f"Warning: Unsupported language {lang} in line: {line}")

        if not has_translation:
            lang_data['en'][key_line] = ""

    # Write JSON files
    for lang, data in lang_data.items():
        if not data:
            continue
        dir_name = f"{lang}.json"
        os.makedirs(dir_name, exist_ok=True)
        file_path = os.path.join(dir_name, 'localize.json')
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(json.dumps(data, ensure_ascii=False, separators=(",\n", ": ")).replace('{', '{\n').replace('}', '\n}'))


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <categories.txt>")
        sys.exit(1)
    input_file = sys.argv[1]
    parse_translations(input_file)


if __name__ == "__main__":
    main()
