#!/usr/bin/env python3
"""
Generate localized_types_map.cpp from LocalizableTypes.strings

This script converts the iOS LocalizableTypes.strings file format to the desktop
localized_types_map.cpp C++ map by:
- Removing comments (/* ... */)
- Removing empty lines
- Converting from "key" = "value"; format to key=value format
- Removing unnecessary quotes and spaces
"""

import re
import os
import sys
from pathlib import Path

def parse_localizable_types_line(line):
    line = line.strip()
    if not line:
        return None
    
    if line.startswith('/*') or line.startswith('//') or line.startswith('/****'):
        return None
    
    # Match pattern: "key" = "value";
    match = re.match(r'^"([^"]+)"\s*=\s*"([^"]*)"\s*;?\s*$', line)
    if match:
        key = match.group(1)
        value = match.group(2)
        return (key, value)
    
    return None


def convert_to_localized_types_cpp(input_file, output_file):
    with open(input_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    entries = []
    for line_num, line in enumerate(lines, 1):
        try:
            parsed = parse_localizable_types_line(line)
            if parsed:
                key, value = parsed
                entries.append((key, value))
        except Exception as e:
            print(f"Warning: Error parsing line {line_num}: {e}")
            print(f"  Line content: {line.strip()}")
            continue

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('#pragma once\n\n')
        f.write('#include <string>\n')
        f.write('#include <unordered_map>\n\n')
        f.write('// This file is generated automatically. Do not edit.\n')
        f.write('// See: tools/python/generate_desktop_ui_strings.py\n')
        f.write('using Type2LocalizedType = std::unordered_map<std::string, std::string>;\n')
        f.write('const Type2LocalizedType g_type2localizedType = {\n')
        for i, (key, value) in enumerate(entries):
            comma = ',' if i < len(entries) - 1 else ''
            f.write(f'  {{"{key}", "{value}"}}{comma}\n')
        f.write('};\n')
    print(f"Successfully converted {len(entries)} entries from '{input_file}' to '{output_file}'")


def main():
    input_file = Path('iphone/Maps/LocalizedStrings/en.lproj/LocalizableTypes.strings')
    output_file = Path('libs/indexer/localized_types_map.cpp')
    convert_to_localized_types_cpp(str(input_file), str(output_file))
    

if __name__ == '__main__':
    sys.exit(main())
