import os

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
    
def main():
    base_directory = os.path.abspath(os.path.join(os.path.realpath(__file__), "..", "..", "..", "iphone",  "Maps", "LocalizedStrings"))
    language_directories = [i[1] for i in os.walk(base_directory)][0]
    language_directories.remove("en.lproj")
    translatable_files = []
    for file in [i[2] for i in os.walk(os.path.join(base_directory, "en.lproj"))][0]:
        if file.endswith(".strings"):
            translatable_files.append(file)
    
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

if __name__ == '__main__':
    main()