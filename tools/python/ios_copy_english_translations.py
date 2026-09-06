#!/usr/bin/env python3
import copy
import os
import plistlib
import re

def read(filepath):
    translations = {}
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            elif '" = "' in line:
                translation_parts = line.split('" = "')
                translations[(translation_parts[0])[1:]] = (translation_parts[1])[:-2]
            elif len(line) > 6 and line[:3] == '/* ' and line[-3:] == ' */':
                comment_line = line
                while comment_line in translations.keys():
                    comment_line = comment_line[:2] + '*' + comment_line[2:]
                translations[comment_line] = ''
    return translations

def write(filepath, translations):
    with open(filepath, 'w') as f:
        for key, value in translations.items():
            if key[:2] == '/*' and key[-2:] == '*/':
                comment = '/* ' + key.split(' ', 1)[1]
                f.write(f'{comment}\n')
            else:
                f.write(f'"{key}" = "{value}";\n')
    
def merge_stringsdict(source, translation, translation_filepath):
    format_key = 'NSStringLocalizedFormatKey'
    variable_key = 'nevertranslate'
    metadata_keys = ('NSStringFormatSpecTypeKey', 'NSStringFormatValueTypeKey')

    for entry_key, source_entry in source.items():
        if entry_key not in translation:
            translation[entry_key] = copy.deepcopy(source_entry)
            continue

        translation_entry = translation[entry_key]
        if not isinstance(translation_entry, dict):
            translation[entry_key] = copy.deepcopy(source_entry)
            continue

        if format_key not in translation_entry:
            translation_entry[format_key] = source_entry[format_key]

        translation_variable = translation_entry.get(variable_key)
        if not isinstance(translation_variable, dict):
            translation_entry[variable_key] = copy.deepcopy(source_entry[variable_key])
        else:
            # Copy required metadata if not present
            # Needed until this PR is deployed to Codeberg Translate: https://github.com/translate/translate/pull/6541
            for metadata_key in metadata_keys:
                if metadata_key not in translation_variable:
                    translation_variable[metadata_key] = source_entry[variable_key][metadata_key]

        # The plural variable name should never be translated as it is structural
        # Needed until we implement a CI check on the Translate PRs before merging
        translation_format = translation_entry[format_key]
        variables = set(re.findall(r'%#@([^@]+)@', translation_format)) if isinstance(translation_format, str) else set()
        if variables != {variable_key}:
            print(f"warning: {translation_filepath}: replacing invalid {format_key} in '{entry_key}': {translation_format!r}")
            translation_entry[format_key] = source_entry[format_key]

def update_stringsdict(source_filepath, translation_filepath):
    with open(source_filepath, 'rb') as f:
        source = plistlib.load(f)
    with open(translation_filepath, 'rb') as f:
        translation = plistlib.load(f)
    merge_stringsdict(source, translation, translation_filepath)
    with open(translation_filepath, 'wb') as f:
        plistlib.dump(translation, f, sort_keys=False)

def main():
    base_directory = os.path.abspath(os.path.join(os.path.realpath(__file__), "..", "..", "..", "iphone",  "Maps", "LocalizedStrings"))
    language_directories = [i[1] for i in os.walk(base_directory)][0]
    language_directories.remove("en.lproj")
    translatable_files = []
    stringsdict_files = []
    for file in [i[2] for i in os.walk(os.path.join(base_directory, "en.lproj"))][0]:
        if file.endswith(".strings"):
            translatable_files.append(file)
        elif file.endswith(".stringsdict"):
            stringsdict_files.append(file)
    
    for translatable_file in translatable_files:
        english_filepath = os.path.join(base_directory, "en.lproj", translatable_file)
        english_translations = read(english_filepath)
        write(english_filepath, english_translations)
        for language_directory in language_directories:
            filepath = os.path.join(base_directory, language_directory, translatable_file)
            if not os.path.exists(filepath):
                translations = english_translations
            else:
                translations = {}
                existing_translations = read(filepath)
                for key, value in english_translations.items():
                    if not key in existing_translations:
                        translations[key] = value
                    else:
                        translations[key] = existing_translations[key]
            write(filepath, translations)

    for stringsdict_file in stringsdict_files:
        english_filepath = os.path.join(base_directory, "en.lproj", stringsdict_file)
        for language_directory in language_directories:
            filepath = os.path.join(base_directory, language_directory, stringsdict_file)
            if os.path.exists(filepath):
                update_stringsdict(english_filepath, filepath)

if __name__ == '__main__':
    main()