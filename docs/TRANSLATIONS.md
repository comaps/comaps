# Translations

Translations are managed through [Codeberg Translate][codeberg_translate], which is based on Weblate. Please [contribute][contribute] translations via the [Codeberg Translate][codeberg_translate], and the system and maintainers will handle the rest.

## Components

The project consists of multiple components, each with its own translation files.

| Weblate Component                                   | Description                                                | Translation Files                                                                                        |
| --------------------------------------------------- | ---------------------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| [Android][android_weblate]                          | UI strings                                                 | [android/app/src/main/res/values\*/strings.xml][android_git] ([en][android_git_en])                      |
| [Android feature types][android_typestrings_weblate] | Map feature types                                        | [android/app/src/main/res/values\*/type_strings.xml][android_git] ([en][android_type_strings_git_en])    |
| [iOS][ios_weblate]                                  | UI strings                                                 | [iphone/Maps/LocalizedStrings/\*.lproj/Localizable.strings][ios_git] ([en][ios_git_en])                  |
| [iOS Type Strings][ios_typestrings_weblate]         | OpenStreetMap Types                                        | [iphone/Maps/LocalizedStrings/\*.lproj/LocalizableTypes.strings][ios_git] ([en][ios_typestrings_git_en]) |
| [iOS Plurals][ios_plurals_weblate]                  | UI strings (plurals)                                       | [iphone/Maps/LocalizedStrings/\*.lproj/Localizable.stringsdict][ios_git] ([en][ios_plurals_git_en])      |
| [iOS Plist][ios_plist_weblate]                      | UI strings (system-level)                                  | [iphone/Maps/LocalizedStrings/\*.lproj/InfoPlist.strings][ios_git] ([en][ios_plist_git_en])              |
| [TTS][tts_weblate]                                  | Voice announcement strings for navigation directions (TTS) | [data/sound-strings/\*.json][tts_git] ([en][tts_git_en])                                                 |
| [Countries][countries_weblate]                      | Country names for downloader                               | [data/country-strings/\*.json][countries_git] ([en][countries_git_en])                                   |
| Search keywords                                     | Search keywords/aliases/synonyms                           | [data/categories.txt][categories_git]                                                                    |
| Search keywords (cuisines)                          | Search keywords for cuisine types                          | [data/categories_cuisines.txt][categories_cuisines_git]                                                  |
| AppStore Descriptions                               | AppStore descriptions                                      | [iphone/metadata][appstore_git] ([en][appstore_git_en])                                                  |
| Android Stores Descriptions                                | Google, F-Droid, Huawei store descriptions                 | [android/app/src/fdroid/play][googleplay_git] ([en][googleplay_git_en])                                  |
| [Website][website_weblate]                          | Website content                                            | [comaps/website][website_git] ([see details][website_guide])                                        |

Components without links haven't been integrated into Weblate and must be translated directly via [Codeberg Pull Requests](CONTRIBUTING.md).

## Translating

### Workflow

Translations are managed through [Codeberg Translate][codeberg_translate]. Direct submissions to this repository are not recommended but possible in specific cases (like batch-changes). Please prefer using the Weblate for translations whenever possible. Weblate periodically creates pull requests, which [@comaps/mergers][mergers] review and merge as usual.

### Cross-Component Synchronization

Android and iOS share most of the strings. Codeberg Translate automatically syncs translations between components (e.g., from Android to iOS and vice versa), so updating a string in one place is usually sufficient.

## Machine Translation

Codeberg Translate is configured to generate machine translations using the best available tools. Auto-translated entries are added as suggestions.

### Failing checks

Please review any issues flagged by automated checks, such as missing placeholders, inconsistencies, and other potential errors. Use the filter [`has:check AND state:>=translated language:de`][failing_checks], replacing `de` with your target language.

## Developing

### Workflow

Translations are handled by the translation team via [**Codeberg Translate**][codeberg_translate], with no direct developer involvement required. Developers are only responsible for adding English base strings to the source file (see [Components](#components)). Codeberg Translate manages the rest. If you're confident in a language, feel free to contribute translations, but please avoid adding machine translations or translating languages you are not familiar with.

### Tools

Android developers can utilize the built-in features of Android Studio to add and modify strings efficiently. iOS developers are advised to edit `Localizable.strings` as a text file, as Xcode’s interface only supports "String Catalog," which is not currently in use. JSON files can be modified using any text editor. To ensure consistency, always follow the established structure and include a comment when adding new strings.

### Cross-Component Synchronization

When adding new strings, first check the base file of the component for existing ones. If no relevant strings are found, look for them on the corresponding platform (e.g., iOS when adding Android strings or vice versa). To maintain consistency across platforms, always reuse the existing string key from the other platform with the same English base string.

## Maintaining

## Under the Hood

Codeberg Translate maintains an internal copy of the Git repository. The repository URL can be found under _Manage → Repository Maintenance → Weblate Repository_. All components, except for the website, share the same internal Weblate repository.

Translations are extracted from the repository and stored in an internal database, which is used by the Weblate UI. Every 24 hours, this internal database is synchronized back to the internal repository. This process can also be triggered manually via _Manage → Repository Maintenance → Commit_.

Codeberg Translate has its own Git repository fork of both the website and the main Git repository for pushing translation commits and then creating pull requests (PRs) on Codeberg. After committing changes from the internal database to the internal repository, Codeberg Translate pushes all updates to the `weblate-comaps-<component>` branch (e.g. `weblate-comaps-android` for Android UI strings) of its forked repository and creates or updates a PR to `main` branch of the main repository. This operation can be manually triggered via _Manage → Repository Maintenance → Push_.

### Reviewing PRs

Translations are intended to be reviewed by the community on Codeberg Translate. However, if it's a user's first contribution or if there is any doubt, a quick scan and comparison with the English source can be useful.

It is recommended to add comments directly on Codeberg Translate, as translators primarily work within that platform. Since Codeberg Translate requires a Codeberg account, you may tag contributors in the pull request, but there is no guarantee that they will respond.

### Resolving Conflicts

The recommended approach for resolving conflicts is as follows:

1. Commit all changes from the internal database to the internal Git repository:  
   _Manage → Repository Maintenance → Commit (button)_.
2. Update the `weblate-comaps-<component>` branch of the Translate's forked Git repository on Codeberg:  
   _Manage → Repository Maintenance → Push (button)_.
3. Locally checkout the `weblate-comaps-<component>` branch.
4. Rebase it onto `main` of the main repository, resolving any conflicts during the process.
5. Push the branch to Codeberg to update the pull request, then merge the branch or PR into `main`.
6. Reset Codeberg Translate to sync changes from Codeberg:  
   _Manage → Repository Maintenance → Reset (button)_.

[codeberg_translate]: https://translate.codeberg.org/projects/comaps/
[contribute]: https://docs.weblate.org/en/latest/workflows.html
[android_weblate]: https://translate.codeberg.org/projects/comaps/android/
[android_git]: https://codeberg.org/comaps/comaps/src/branch/main/android/app/src/main/res
[android_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/android/app/src/main/res/values/strings.xml
[android_typestrings_weblate]: https://translate.codeberg.org/projects/comaps/android-typestrings/
[android_typestrings_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/android/app/src/main/res/values/types_strings.xml
[countries_weblate]: https://translate.codeberg.org/projects/comaps/countries/
[countries_git]: https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings
[countries_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/data/countries-strings/en.json/localize.json
[ios_weblate]: https://translate.codeberg.org/projects/comaps/ios/
[ios_git]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/Maps/LocalizedStrings/
[ios_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/Maps/LocalizedStrings/en.lproj/Localizable.strings
[ios_plist_weblate]: https://translate.codeberg.org/projects/comaps/ios-plist/
[ios_plist_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/Maps/LocalizedStrings/en.lproj/InfoPlist.strings
[ios_typestrings_weblate]: https://translate.codeberg.org/projects/comaps/ios-typestrings/
[ios_typestrings_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/Maps/LocalizedStrings/en.lproj/LocalizableTypes.strings
[ios_plurals_weblate]: https://translate.codeberg.org/projects/comaps/ios-plurals/
[ios_plurals_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/Maps/LocalizedStrings/en.lproj/Localizable.stringsdict
[tts_weblate]: https://translate.codeberg.org/projects/comaps/tts/
[tts_git]: https://codeberg.org/comaps/comaps/src/branch/main/data/sound-strings
[tts_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/data/sound-strings/en.json/localize.json
[categories_git]: https://codeberg.org/comaps/comaps/src/branch/main/data/categories.txt
[categories_cuisines_git]:https://codeberg.org/comaps/comaps/src/branch/main/data/categories_cuisines.txt
[website_weblate]: https://translate.codeberg.org/projects/comaps/website/
[website_git]: https://codeberg.org/comaps/website/
[website_guide]: https://codeberg.org/comaps/website/src/branch/main/TRANSLATIONS.md
[appstore_git]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/metadata
[appstore_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/iphone/metadata/en-US
[googleplay_git]: https://codeberg.org/comaps/comaps/src/branch/main/android/app/src/fdroid/play
[googleplay_git_en]: https://codeberg.org/comaps/comaps/src/branch/main/android/app/src/fdroid/play/listings/en-US
[mergers]: https://codeberg.org/org/comaps/teams
[failing_checks]: https://translate.codeberg.org/search/comaps/?q=has%3Acheck+AND+state%3A%3E%3Dtranslated+language%3Aru&sort_by=target&checksum=
