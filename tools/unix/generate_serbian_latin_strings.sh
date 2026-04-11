#!/usr/bin/env sh

echo "Converting Serbian Cyrillic resource files to Serbian Latin"

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ANDROID="$REPO_ROOT/android"

convert() {
  INPUT="$1"
  OUTPUT="$2"
  uconv -x 'Serbian-Latin/BGN' "$INPUT" > "$OUTPUT"
}

mkdir -p "android/app/src/fdroid/play/listings/sr-Latn"
convert "$ANDROID/app/src/fdroid/play/listings/sr/full-description.txt" \
        "$ANDROID/app/src/fdroid/play/listings/sr-Latn/full-description.txt"
convert "$ANDROID/app/src/fdroid/play/listings/sr/short-description.txt" \
        "$ANDROID/app/src/fdroid/play/listings/sr-Latn/short-description.txt"
convert "$ANDROID/app/src/fdroid/play/listings/sr/title.txt" \
        "$ANDROID/app/src/fdroid/play/listings/sr-Latn/title.txt"

mkdir -p "$ANDROID/app/src/main/res/values-b+sr+Latn"
convert "$ANDROID/app/src/main/res/values-sr/strings.xml" \
        "$ANDROID/app/src/main/res/values-b+sr+Latn/strings.xml"

mkdir -p "$ANDROID/sdk/src/main/res/values-b+sr+Latn"
convert "$ANDROID/sdk/src/main/res/values-sr/strings.xml" \
        "$ANDROID/sdk/src/main/res/values-b+sr+Latn/strings.xml"
convert "$ANDROID/sdk/src/main/res/values-sr/types_strings.xml" \
        "$ANDROID/sdk/src/main/res/values-b+sr+Latn/types_strings.xml"

IPHONE_SR="$REPO_ROOT/iphone/Maps/LocalizedStrings/sr.lproj"
IPHONE_SR_LATN="$REPO_ROOT/iphone/Maps/LocalizedStrings/sr-Latn.lproj"

mkdir -p "$IPHONE_SR_LATN"
convert "$IPHONE_SR/Localizable.strings" \
        "$IPHONE_SR_LATN/Localizable.strings"
convert "$IPHONE_SR/InfoPlist.strings" \
        "$IPHONE_SR_LATN/InfoPlist.strings"
convert "$IPHONE_SR/LocalizableTypes.strings" \
        "$IPHONE_SR_LATN/LocalizableTypes.strings"
convert "$IPHONE_SR/Localizable.stringsdict" \
        "$IPHONE_SR_LATN/Localizable.stringsdict"

mkdir -p "$REPO_ROOT/data/countries-strings/sr_Latn.json"
convert "$REPO_ROOT/data/countries-strings/sr.json/localize.json" \
        "$REPO_ROOT/data/countries-strings/sr_Latn.json/localize.json"
