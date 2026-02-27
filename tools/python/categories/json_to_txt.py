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


def load_localize_json(lang_dir):
    file_path = os.path.join(lang_dir, 'localize.json')
    if not os.path.isfile(file_path):
        return {}
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON from {file_path}: {e}")
        return {}


def collect_all_keys(base_dir):
    all_data = {}
    lang_dirs = [d for d in os.listdir(base_dir) if d.endswith('.json')]

    for lang_dir in lang_dirs:
        lang = lang_dir.replace('.json', '')
        if lang not in LANGUAGES:
            print(f"Skipping unsupported language directory: {lang_dir}")
            continue
        full_path = os.path.join(base_dir, lang_dir)
        if os.path.isdir(full_path):
            data = load_localize_json(full_path)
            for key, value in data.items():
                if key not in all_data:
                    all_data[key] = {}
                all_data[key][lang] = value

    return all_data


def write_category_file(all_data, output_file):
    with open(output_file, 'w', encoding='utf-8') as f:
        for i, (key, translations) in enumerate(all_data.items()):
            f.write(key + '\n')
            for lang in LANGUAGES:
                if lang in translations and translations[lang]:
                    f.write(f"{lang}:{translations[lang]}\n")
                elif lang == 'en' and key in translations:
                    f.write('\n')
            if i < len(all_data) - 1:
                f.write('\n')


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <json_directory> [categories.txt]")
        sys.exit(1)

    base_dir = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else "categories.txt"

    if not os.path.isdir(base_dir):
        print(f"Directory not found: {base_dir}")
        sys.exit(1)

    all_data = dict(sorted(collect_all_keys(base_dir).items()))
    write_category_file(all_data, output_file)


if __name__ == "__main__":
    main()
