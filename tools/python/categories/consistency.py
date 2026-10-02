#!/usr/bin/env python3
import sys
import os
import re
import json
from string import digits

CONTENT_REGEX = re.compile(r'/\*.*?\*/', re.DOTALL)
CAT_ENTRIES_REGEX = re.compile(r'"(category_.*?)"\s*=\s*"(.*?)"')
TYPE_ENTRIES_REGEX = re.compile(r'"(.*?)"\s*=\s*"(.*?)"')
SINGLE_REPLACE = False

def fix_predefined_categories(lang):
    strings_file_path = os.path.join('iphone', 'Maps', 'LocalizedStrings', f'{lang}.lproj', 'Localizable.strings')
    if not os.path.isfile(strings_file_path):
        print(f'{strings_file_path} doesn\'t exist')
        return

    json_file_path = os.path.join('data', 'translations', 'categories-strings', f'{lang}.json', 'localize.json')
    if not os.path.isfile(json_file_path):
        print(f'{json_file_path} doesn\'t exist')
        return

    with open(strings_file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove comments
    content = re.sub(CONTENT_REGEX, '', content)

    cats_entries = {key: value for key, value in re.findall(CAT_ENTRIES_REGEX, content)}

    with open(json_file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    for cat_name, localized_value in cats_entries.items():
        cat_name = '@'+cat_name
        key_matched = False
        for json_key in data.keys():
            if json_key.startswith('@category_'):
                _key_matched = False

                if cat_name == json_key:
                    key_matched = True
                    data_split = data[json_key]['defaultMessage'].split('|')

                    for value in data_split:
                        if value and value[0] in digits:
                            value = value[1:]

                        value = value.lower()
                        localized_value_lower = localized_value.lower()

                        if localized_value_lower == value:
                            _key_matched = True
                            break

                    if not _key_matched:
                        data[json_key]['defaultMessage'] = localized_value+'|'+data[json_key]['defaultMessage']
                        print(f'Prepended "{localized_value}" to "{json_key}"')

        if not key_matched:
            print(f'Created "{cat_name}: {localized_value}"')
            data[cat_name] = {'defaultMessage': localized_value}

    res = json.dumps(data, ensure_ascii=False, separators=(",", ": "), indent=4) + '\n'

    with open(json_file_path, 'w', encoding='utf-8') as f:
        f.write(res)


def main(lang, data_en):
    strings_file_path = os.path.join('iphone', 'Maps', 'LocalizedStrings', f'{lang}.lproj', 'LocalizableTypes.strings')
    json_file_path = os.path.join('data', 'translations', 'categories-strings', f'{lang}.json', 'localize.json')

    with open(strings_file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove comments
    content = re.sub(CONTENT_REGEX, '', content)

    type_entries = {key[5:]: value for key, value in re.findall(TYPE_ENTRIES_REGEX, content)}

    with open(json_file_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    for type_name, localized_value in type_entries.items():
        key_matched = False
        for json_key in data.keys():
            json_key_split = json_key.split('|')
            for key in json_key_split:
                already_there = False
                _key_matched = False

                if type_name == key.replace('-', '.').replace('_', '.'):
                    key_matched = True
                    data_split = data[json_key]['defaultMessage'].split('|')

                    try:
                        data_split.extend([
                                            value
                                            for category in
                                            [a for a in json_key_split
                                                if a.startswith('@')]
                                            for value in
                                            data[category]['defaultMessage'].split('|')
                                        ])
                    except KeyError:
                        pass

                    for value in data_split:
                        if value and value[0] in digits:
                            value = value[1:]

                        value = value.lower()
                        localized_value_lower = localized_value.lower()

                        # Prevents adding duplicates that differ only by the word "shop"
                        if value in localized_value_lower:
                            already_there = True
                            break

                        if localized_value_lower == value:
                            _key_matched = True
                            break

                    if already_there:
                        break

                    if not _key_matched:
                        if SINGLE_REPLACE and len(data_split) == 1:
                            data[json_key]['defaultMessage'] = localized_value
                            print(f'Replaced "{data[json_key]['defaultMessage']}" with "{localized_value}" in "{json_key}"')

                        else:
                            data[json_key]['defaultMessage'] = localized_value+'|'+data[json_key]['defaultMessage']
                            print(f'Appended "{localized_value}" to "{json_key}"')

        if not key_matched:
            for json_key in data.keys():
                for key in json_key.split('|'):
                    if type_name == key.replace('-', '.').replace('_', '.'):
                        print(f'Created "{localized_value}" for "{json_key}"')
                        data.update({json_key: {'defaultMessage': localized_value}})

    res = json.dumps(data, ensure_ascii=False, separators=(",", ": "), indent=4) + '\n'

    with open(json_file_path, 'w', encoding='utf-8') as f:
        f.write(res)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} [-c] [-r] <language_codes>")
        sys.exit(1)

    if sys.argv[1] == '-c':
        del sys.argv[1]
        if len(sys.argv) < 2:
            print("No languages specified")
            sys.exit(1)
        for lang in sys.argv[1:]:
            print(f'{lang}:')
            fix_predefined_categories(lang)
            print('\n')
        sys.exit(0)

    if sys.argv[1] == '-r':
        SINGLE_REPLACE = True
        del sys.argv[1]
        if len(sys.argv) < 2:
            print("No languages specified")
            sys.exit(1)

    with open('data/translations/categories-strings/en.json/localize.json', 'r', encoding='utf-8') as f:
        data_en = json.load(f)

    if len(sys.argv) > 2:
        for lang in sys.argv[1:]:
            print(f'{lang}:')
            main(lang, data_en)
            print('\n')
    else:
        main(sys.argv[1], data_en)
