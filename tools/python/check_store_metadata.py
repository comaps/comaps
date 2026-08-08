#!/usr/bin/env python3
#
# Check AppStore / GooglePlay / F-Droid metadata
#

import os
import sys
import glob
import shutil
from argparse import ArgumentParser
from urllib.parse import urlparse

os.chdir(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", ".."))

ANDROID_META_PATH='android/app/src'
# https://support.google.com/googleplay/android-developer/answer/9844778?visit_id=637740303439369859-3116807078&rd=1#zippy=%2Cview-list-of-available-languages
GPLAY_LOCALES = [
    "af",    # Afrikaans
    "sq",    # Albanian
    "am",    # Amharic
    "ar",    # Arabic
    "hy-AM", # Armenian
    "az-AZ", # Azerbaijani
    "bn-BD", # Bangla
    "eu-ES", # Basque
    "be",    # Belarusian
    "bg",    # Bulgarian
    "my-MM", # Burmese
    "ca",    # Catalan
    "zh-HK", # Chinese (Hong Kong)
    "zh-CN", # Chinese (Simplified)
    "zh-TW", # Chinese (Traditional)
    "hr",    # Croatian
    "cs-CZ", # Czech
    "da-DK", # Danish
    "nl-NL", # Dutch
    "en-IN", # English
    "en-SG", # English
    "en-ZA", # English
    "en-AU", # English (Australia)
    "en-CA", # English (Canada)
    "en-GB", # English (United Kingdom)
    "en-US", # English (United States)
    "et",    # Estonian
    "fil",   # Filipino
    "fi-FI", # Finnish
    "fr-CA", # French (Canada)
    "fr-FR", # French (France)
    "gl-ES", # Galician
    "ka-GE", # Georgian
    "de-DE", # German
    "el-GR", # Greek
    "gu",    # Gujarati
    "iw-IL", # Hebrew
    "hi-IN", # Hindi
    "hu-HU", # Hungarian
    "is-IS", # Icelandic
    "id",    # Indonesian
    "it-IT", # Italian
    "ja-JP", # Japanese
    "kn-IN", # Kannada
    "kk",    # Kazakh
    "km-KH", # Khmer
    "ko-KR", # Korean
    "ky-KG", # Kyrgyz
    "lo-LA", # Lao
    "lv",    # Latvian
    "lt",    # Lithuanian
    "mk-MK", # Macedonian
    "ms",    # Malay
    "ms-MY", # Malay (Malaysia)
    "ml-IN", # Malayalam
    "mr-IN", # Marathi
    "mn-MN", # Mongolian
    "ne-NP", # Nepali
    "no-NO", # Norwegian
    "fa",    # Persian
    "fa-AE", # Persian
    "fa-AF", # Persian
    "fa-IR", # Persian
    "pl-PL", # Polish
    "pt-BR", # Portuguese (Brazil)
    "pt-PT", # Portuguese (Portugal)
    "pa",    # Punjabi
    "ro",    # Romanian
    "rm",    # Romansh
    "ru-RU", # Russian
    "sr",    # Serbian
    "si-LK", # Sinhala
    "sk",    # Slovak
    "sl",    # Slovenian
    "es-419", # Spanish (Latin America)
    "es-ES", # Spanish (Spain)
    "es-US", # Spanish (United States)
    "sw",    # Swahili
    "sv-SE", # Swedish
    "ta-IN", # Tamil
    "te-IN", # Telugu
    "th",    # Thai
    "tr-TR", # Turkish
    "uk",    # Ukrainian
    "ur",    # Urdu
    "vi",    # Vietnamese
    "zu",    # Zulu
]

IOS_META_PATH='iphone/metadata'
# From a Fastline error message and https://help.apple.com/app-store-connect/#/dev997f9cf7c
APPSTORE_LOCALES = [
    "ar-SA", "ca", "cs", "da", "de-DE", "el", "en-AU", "en-CA", "en-GB", "en-US", "es-ES", "es-MX", "fi", "fr-CA", "fr-FR", "he", "hi", "hr", "hu", "id", "it", "ja", "ko", "ms", "nl-NL", "no", "pl", "pt-BR", "pt-PT", "ro", "ru", "sk", "sv", "th", "tr", "uk", "vi", "zh-Hans", "zh-Hant"
]

verbose = False

def error(path, message, *args, **kwargs):
    print(f'ERROR: {path}', message.format(*args, **kwargs), file=sys.stderr)
    return False


def done(path, ok):
    if ok and verbose:
        print(f'OK: {path}')
    return ok

def check_raw(path, max_length, ignoreEmptyFilesAndNewLines=False):
    ok = True
    with open(path, 'r') as f:
        text = f.read()
        if not ignoreEmptyFilesAndNewLines:
            if not text:
                ok = error(path, "empty")
            elif text[-1] == os.linesep:
                text = text[:-1]
            else:
                ok = error(path, "missing new line")
        else:
            text = text.strip()

        cur_length = len(text)
        if cur_length > max_length:
            ok = error(path, f'too long {cur_length} (max {max_length})')
        return ok, text

def check_text(path, max, optional=False, ignoreEmptyFilesAndNewLines=False):
    try:
        return done(path, check_raw(path, max, ignoreEmptyFilesAndNewLines)[0])
    except FileNotFoundError as e:
        if optional:
            return True
        error(path, 'not exists')
        return False

def check_url(path, ignoreEmptyFilesAndNewLines=False):
    if not os.path.exists(path):
        error(path, 'not exists')
        return False
    (ok, url) = check_raw(path, 500, ignoreEmptyFilesAndNewLines)
    url = urlparse(url)
    if not url.scheme in ('https', 'http'):
        ok = error(path, "invalid URL: {}", url)
    return done(path, ok)

def check_email(path):
    (ok, email) = check_raw(path, 500)
    ok = ok and email.find('@') != -1 and email.find('.') != -1
    return done(path, ok)

def check_exact(path, expected):
    (ok, value) = check_raw(path, len(expected))
    if value != expected:
        ok = error(path, "invalid value: got={}, expected={}", value, expected)
    return done(path, ok)

def drop_locale(locale_path):
    shutil.rmtree(locale_path)
    print(f'REMOVED invalid locale {locale_path}')


def check_android(is_gplay, fix=False):
    ok = True
    flavor = "google" if is_gplay else "fdroid"
    flavor = f'{ANDROID_META_PATH}/{flavor}/play/'
    ok = check_url(flavor + 'contact-website.txt') and ok
    ok = check_email(flavor + 'contact-email.txt') and ok
    ok = check_exact(flavor + 'default-language.txt', 'en-US') and ok
    for locale in glob.glob(flavor + 'listings/*/'):
        if is_gplay and locale.split('/')[-2] not in GPLAY_LOCALES:
            error(locale, 'unsupported locale')
            if fix:
                drop_locale(locale)
            ok = False
        else:
            locale_ok = check_text(locale + 'title.txt', 30 if is_gplay else 50)
            desc_path = locale + 'short-description.txt'
            locale_ok = check_text(desc_path, 80) and locale_ok
            if is_gplay and os.path.exists(desc_path):
                desc = open(desc_path, 'r').read()
                if '--' in desc or '–' in desc:
                    # The app may not be promoted on Google Play because of the following
                    print(f'WARN: {desc_path} should use em dashes instead of double hyphens or en dashes')
                    if fix:
                        desc = desc.replace('--', '—').replace('–', '—')
                        open(desc_path, 'w').write(desc)
                        print(f'REPLACED wrong dashes in {desc_path}')
            locale_ok = check_text(locale + 'full-description.txt', 4000) and locale_ok
            locale_ok = check_text(locale + 'release-notes.txt', 499, optional=True) and locale_ok
            done(locale, locale_ok)
            if not locale_ok:
                error(locale, 'locale is INVALID or INCOMPLETE')
                if fix:
                    drop_locale(locale)
                ok = False
    ''' TODO: relnotes not necessary exist for all languages, but symlinks are made for all
    for locale in glob.glob(flavor + 'release-notes/*/'):
        if locale.split('/')[-2] not in GPLAY_LOCALES:
            ok = error(locale, 'unsupported locale') and ok
            continue
        ok = check_text(locale + 'default.txt', 499) and ok
    '''
    if not ok:
        if fix:
            print(f'FIXED by removing invalid locales from {flavor}')
            return True
        else:
            error(flavor, 'HAS INVALID LOCALES')
            return False
    else:
        print(f'metadata is OK {flavor}')
        return True


def check_ios(fix=False):
    ok = True
    for locale in glob.glob(f'{IOS_META_PATH}/*/'):
        if locale.split('/')[-2] not in APPSTORE_LOCALES:
            error(locale, "unsupported locale")
            if fix:
                drop_locale(locale)
            ok = False
        else:
            locale_ok = check_text(locale + "subtitle.txt", 30, False, True)
            locale_ok = check_text(locale + "description.txt", 4000, False, True) and locale_ok
            locale_ok = check_text(locale + "keywords.txt", 100, False, True) and locale_ok
            locale_ok = check_text(locale + "release_notes.txt", 4000, True, True) and locale_ok
            locale_ok = check_url(locale + "support_url.txt", True) and locale_ok
            locale_ok = check_url(locale + "marketing_url.txt", True) and locale_ok
            locale_ok = check_url(locale + "privacy_url.txt", True) and locale_ok
            done(locale, locale_ok)
            if not locale_ok:
                error(locale, 'locale is INVALID or INCOMPLETE')
                if fix:
                    drop_locale(locale)
                ok = False

    if not ok:
        if fix:
            print(f'FIXED by removing invalid locales from {IOS_META_PATH}')
            return True
        else:
            error(IOS_META_PATH, 'HAS INVALID LOCALES')
            return False
    else:
        print(f'metadata is OK {IOS_META_PATH}')
        return True

if __name__ == "__main__":
    parser = ArgumentParser(description="Check AppStore / Google Play / F-Droid metadata")
    parser.add_argument("platform", choices=["gplay", "fdroid", "ios"], help="store metadata to check")
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose output")
    parser.add_argument("-f", "--fix", action="store_true", help="remove invalid/incomplete locales")
    args = parser.parse_args()
    verbose = args.verbose
    to_fix = args.fix
    if args.platform == 'gplay':
        sys.exit(0 if check_android(is_gplay=True, fix=to_fix) else 2)
    elif args.platform == 'fdroid':
        sys.exit(0 if check_android(is_gplay=False, fix=to_fix) else 2)
    elif args.platform == "ios":
        sys.exit(0 if check_ios(fix=to_fix) else 2)
