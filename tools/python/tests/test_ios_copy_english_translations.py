import plistlib
import tempfile
import unittest
from pathlib import Path

from tools.python import ios_copy_english_translations as translations


class MergeMissingTests(unittest.TestCase):
    def test_adds_missing_plural_without_replacing_translation(self):
        source = {
            "capacity": {
                "NSStringLocalizedFormatKey": "%#@nevertranslate@",
                "nevertranslate": {
                    "NSStringFormatSpecTypeKey": "NSStringPluralRuleType",
                    "NSStringFormatValueTypeKey": "lld",
                    "one": "%lld space",
                    "other": "%lld spaces",
                },
            }
        }
        translation = {
            "capacity": {
                "nevertranslate": {
                    "NSStringFormatSpecTypeKey": "NSStringPluralRuleType",
                    "NSStringFormatValueTypeKey": "lld",
                    "one": "%lld Stellplatz",
                    "other": "%lld Stellplätze",
                }
            }
        }

        self.assertTrue(translations.merge_missing(source, translation))
        self.assertEqual(
            translation["capacity"]["NSStringLocalizedFormatKey"],
            "%#@nevertranslate@",
        )
        self.assertEqual(
            translation["capacity"]["nevertranslate"]["other"],
            "%lld Stellplätze",
        )

    def test_preserves_locale_specific_plural_categories(self):
        source = {"count": {"forms": {"one": "one", "other": "other"}}}
        translation = {
            "count": {
                "forms": {
                    "one": "uno",
                    "few": "pochi",
                    "other": "molti",
                }
            }
        }

        self.assertFalse(translations.merge_missing(source, translation))
        self.assertEqual(translation["count"]["forms"]["few"], "pochi")

    def test_copies_missing_entry_by_value(self):
        source = {"capacity": {"forms": {"other": "%lld spaces"}}}
        translation = {}

        self.assertTrue(translations.merge_missing(source, translation))
        self.assertEqual(translation, source)
        translation["capacity"]["forms"]["other"] = "changed"
        self.assertEqual(source["capacity"]["forms"]["other"], "%lld spaces")

    def test_stringsdict_update_is_idempotent(self):
        source = {"capacity": {"format": "value"}}
        target = {}
        with tempfile.TemporaryDirectory() as directory:
            source_path = Path(directory) / "source.stringsdict"
            target_path = Path(directory) / "target.stringsdict"
            source_path.write_bytes(
                plistlib.dumps(source, fmt=plistlib.FMT_XML, sort_keys=False)
            )
            target_path.write_bytes(
                plistlib.dumps(target, fmt=plistlib.FMT_XML, sort_keys=False)
            )

            serialized, changed = translations.update_stringsdict(
                source_path, target_path
            )
            self.assertTrue(changed)
            target_path.write_bytes(serialized)
            _, changed = translations.update_stringsdict(source_path, target_path)
            self.assertFalse(changed)

    def test_regenerate_skips_missing_plural_resource(self):
        with tempfile.TemporaryDirectory() as directory:
            base_path = Path(directory)
            english_path = base_path / "en.lproj"
            translation_path = base_path / "de.lproj"
            english_path.mkdir()
            translation_path.mkdir()
            (english_path / "Localizable.stringsdict").write_bytes(
                plistlib.dumps({"count": {"other": "%lld items"}})
            )

            changed = translations.regenerate(base_path, "stringsdict")

            self.assertEqual(changed, [])
            self.assertFalse(
                (translation_path / "Localizable.stringsdict").exists()
            )

    def test_default_regeneration_processes_strings_and_stringsdict(self):
        with tempfile.TemporaryDirectory() as directory:
            base_path = Path(directory)
            english_path = base_path / "en.lproj"
            translation_path = base_path / "de.lproj"
            english_path.mkdir()
            translation_path.mkdir()
            (english_path / "Localizable.strings").write_text(
                '"title" = "Title";\n', encoding="utf-8"
            )
            (translation_path / "Localizable.strings").write_text(
                "", encoding="utf-8"
            )
            (english_path / "Localizable.stringsdict").write_bytes(
                plistlib.dumps({"count": {"other": "%lld items"}})
            )
            (translation_path / "Localizable.stringsdict").write_bytes(
                plistlib.dumps({})
            )

            changed = translations.regenerate(base_path)

            self.assertEqual(len(changed), 2)
            self.assertIn(
                '"title" = "Title";',
                (translation_path / "Localizable.strings").read_text(
                    encoding="utf-8"
                ),
            )
            stringsdict_path = translation_path / "Localizable.stringsdict"
            with stringsdict_path.open("rb") as file:
                self.assertIn("count", plistlib.load(file))


if __name__ == "__main__":
    unittest.main()
