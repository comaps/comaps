import importlib.util
import io
import json
import os
import tempfile
import unittest
import unittest.mock
import zipfile

_HERE = os.path.dirname(__file__)
_PREPROCESSOR = os.path.normpath(
    os.path.join(_HERE, "..", "generator", "openaddresses_preprocessor.py")
)
_spec = importlib.util.spec_from_file_location("openaddresses_preprocessor", _PREPROCESSOR)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)

_find_address_geojsons       = _mod._find_address_geojsons
_source_key_from_geojson_path = _mod._source_key_from_geojson_path
_license_is_odbl_compatible   = _mod._license_is_odbl_compatible
_is_odbl_compatible_source    = _mod._is_odbl_compatible_source
_load_source_json             = _mod._load_source_json
_get_source_attribution       = _mod._get_source_attribution
_process                      = _mod.process


class TestFindAddressGeojsons(unittest.TestCase):
    def _make_zip(self, names):
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            for name in names:
                zf.writestr(name, "{}")
        buf.seek(0)
        return zipfile.ZipFile(buf)

    def test_finds_canada_countrywide(self):
        zf = self._make_zip(["ca/countrywide.geojson"])
        self.assertEqual(_find_address_geojsons(zf), ["ca/countrywide.geojson"])

    def test_finds_multiple_countrywide(self):
        zf = self._make_zip(["ca/countrywide.geojson", "au/countrywide.geojson"])
        self.assertCountEqual(
            _find_address_geojsons(zf),
            ["ca/countrywide.geojson", "au/countrywide.geojson"],
        )

    def test_finds_us_county_addresses(self):
        zf = self._make_zip(["us/or/marion-addresses-county.geojson"])
        self.assertEqual(
            _find_address_geojsons(zf), ["us/or/marion-addresses-county.geojson"]
        )

    def test_finds_us_statewide_addresses(self):
        zf = self._make_zip(["us/or/statewide-addresses-state.geojson"])
        self.assertEqual(
            _find_address_geojsons(zf), ["us/or/statewide-addresses-state.geojson"]
        )

    def test_finds_multiple_us_counties(self):
        zf = self._make_zip([
            "us/or/marion-addresses-county.geojson",
            "us/or/washington-addresses-county.geojson",
        ])
        self.assertCountEqual(_find_address_geojsons(zf), [
            "us/or/marion-addresses-county.geojson",
            "us/or/washington-addresses-county.geojson",
        ])

    def test_finds_depth2_addresses_without_countrywide(self):
        zf = self._make_zip(["ca/bc/victoria-addresses-county.geojson"])
        self.assertEqual(
            _find_address_geojsons(zf),
            ["ca/bc/victoria-addresses-county.geojson"],
        )

    def test_ignores_meta_files(self):
        zf = self._make_zip(["meta.json", "ca/countrywide.geojson"])
        self.assertEqual(_find_address_geojsons(zf), ["ca/countrywide.geojson"])

    def test_ignores_non_address_geojsons(self):
        zf = self._make_zip(["ca/buildings.geojson", "ca/parcels.geojson"])
        with self.assertRaises(ValueError):
            _find_address_geojsons(zf)

    def test_raises_when_empty_zip(self):
        zf = self._make_zip([])
        with self.assertRaises(ValueError):
            _find_address_geojsons(zf)

    def test_ignores_non_geojson_extension(self):
        zf = self._make_zip(["ca/countrywide.csv"])
        with self.assertRaises(ValueError):
            _find_address_geojsons(zf)


class TestSourceKeyFromGeojsonPath(unittest.TestCase):
    def test_county_path(self):
        self.assertEqual(
            _source_key_from_geojson_path("ca/bc/city_of_victoria-addresses-county.geojson"),
            "ca/bc/city_of_victoria",
        )

    def test_city_path(self):
        self.assertEqual(
            _source_key_from_geojson_path("us/wa/city_of_spokane-addresses-city.geojson"),
            "us/wa/city_of_spokane",
        )

    def test_countrywide_path(self):
        self.assertEqual(
            _source_key_from_geojson_path("ca/countrywide.geojson"),
            "ca/countrywide",
        )

    def test_no_layer_suffix(self):
        self.assertEqual(
            _source_key_from_geojson_path("ca/bc/vancouver.geojson"),
            "ca/bc/vancouver",
        )


class TestLicenseIsOdblCompatible(unittest.TestCase):
    def _check(self, license_info):
        return _license_is_odbl_compatible(license_info)

    def test_clean_url_is_compatible(self):
        self.assertTrue(self._check({"url": "https://creativecommons.org/licenses/by/4.0/"}))

    def test_nc_url_is_blocked(self):
        self.assertFalse(self._check({"url": "https://creativecommons.org/licenses/by-nc/4.0/"}))

    def test_nd_url_is_blocked(self):
        self.assertFalse(self._check({"url": "https://creativecommons.org/licenses/by-nd/4.0/"}))

    def test_nc_in_text_is_blocked(self):
        self.assertFalse(self._check({"url": "https://example.com/license", "text": "non-commercial use only"}))

    def test_nd_in_text_is_blocked(self):
        self.assertFalse(self._check({"url": "", "text": "no derivatives allowed"}))

    def test_bare_string_clean_is_compatible(self):
        self.assertTrue(self._check("https://opendatacommons.org/licenses/odbl/"))

    def test_bare_string_nc_is_blocked(self):
        self.assertFalse(self._check("https://creativecommons.org/licenses/by-nc-sa/4.0/"))

    def test_odbl_url_is_compatible(self):
        self.assertTrue(self._check({"url": "https://opendatacommons.org/licenses/odbl/1.0/"}))

    def test_sa_url_is_blocked(self):
        self.assertFalse(self._check({"url": "https://creativecommons.org/licenses/by-sa/4.0/"}))

    def test_sa_url_is_blocked_v3(self):
        self.assertFalse(self._check({"url": "https://creativecommons.org/licenses/by-sa/3.0/",
                                      "text": "CC BY-SA 3.0"}))

    def test_nc_sa_url_is_blocked(self):
        self.assertFalse(self._check({"url": "https://creativecommons.org/licenses/by-nc-sa/4.0/"}))


class TestLicenseBlacklistAgainstOaSoures(unittest.TestCase):
    """Validate the license blacklist against the full set of real OA source licenses.

    Requires a local clone of https://github.com/openaddresses/openaddresses.
    Set OA_SOURCES_DIR to the path, otherwise the test is skipped.
    """

    @classmethod
    def _collect_real_licenses(cls, oa_sources_dir):
        import os as _os

        licenses = set()
        sources_root = _os.path.join(oa_sources_dir, "sources")
        if not _os.path.isdir(sources_root):
            raise FileNotFoundError(f"OA sources dir not found: {sources_root}")

        for root, _dirs, files in _os.walk(sources_root):
            for f in files:
                if not f.endswith(".json"):
                    continue
                path = _os.path.join(root, f)
                try:
                    with open(path) as fh:
                        data = json.load(fh)
                except Exception:
                    continue
                entries = data if isinstance(data, list) else [data]
                for entry in entries:
                    if not isinstance(entry, dict):
                        continue
                    for layer_list in entry.get("layers", {}).values():
                        if not isinstance(layer_list, list):
                            layer_list = [layer_list]
                        for layer in layer_list:
                            if not isinstance(layer, dict):
                                continue
                            lic = layer.get("license")
                            if lic:
                                if isinstance(lic, str):
                                    licenses.add(lic)
                                elif isinstance(lic, dict):
                                    licenses.add((lic.get("url", ""), lic.get("text", "")))
        return licenses

    KNOWN_COMPATIBLE = {
        "https://creativecommons.org/licenses/by/4.0/",
        "https://creativecommons.org/licenses/by/3.0/",
        "https://creativecommons.org/publicdomain/zero/1.0/",
        "https://opendatacommons.org/licenses/odbl/1.0/",
        "https://opendatacommons.org/licenses/pddl/1-0/",
        "https://www.openstreetmap.org/copyright",
        "https://alliance.numerique.gouv.fr/licence-ouverte-open-licence/",
        "https://www.dati.gov.it/iodl/2.0/",
        "https://www.govdata.de/dl-de/by-2-0",
        "https://www.govdata.de/dl-de/zero-2-0",
        "https://data.gov.tw/license",
        "https://datos.gob.mx/libreusomx/",
        "https://creativecommons.org/licenses/by/3.0/au/",
        "https://data.norge.no/nlod/no/1.0",
        "https://www.statcan.gc.ca/en/reference/licence",
        "https://opendata.vancouver.ca/pages/licence/",
        "https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/",
        "https://data.edmonton.ca/stories/s/City-of-Edmonton-Open-Data-Terms-of-Use/msh8-if28/",
    }

    KNOWN_INCOMPATIBLE = {
        "https://creativecommons.org/licenses/by-nc/4.0/",
        "https://creativecommons.org/licenses/by-nd/4.0/",
        "https://creativecommons.org/licenses/by-nc-sa/4.0/",
        "https://creativecommons.org/licenses/by-sa/4.0/",
        "https://creativecommons.org/licenses/by-sa/3.0/",
    }

    def test_known_compatible_not_blocked(self):
        for lic_url in self.KNOWN_COMPATIBLE:
            with self.subTest(license=lic_url):
                self.assertTrue(
                    _license_is_odbl_compatible({"url": lic_url}),
                    f"Known-compatible license was BLOCKED: {lic_url}"
                )

    def test_known_incompatible_are_blocked(self):
        for lic_url in self.KNOWN_INCOMPATIBLE:
            with self.subTest(license=lic_url):
                self.assertFalse(
                    _license_is_odbl_compatible({"url": lic_url}),
                    f"Known-incompatible license was ALLOWED: {lic_url}"
                )

    @unittest.skipUnless(os.environ.get("OA_SOURCES_DIR"), "Set OA_SOURCES_DIR to run full license audit")
    def test_all_real_licenses_no_false_positives(self):
        """Every real OA license is either marked compatible or there's a reason it's not."""
        oa_dir = os.environ["OA_SOURCES_DIR"]
        licenses = self._collect_real_licenses(oa_dir)

        for lic in licenses:
            if isinstance(lic, tuple):
                lic_obj = {"url": lic[0], "text": lic[1]}
                combined = (lic[0] + " " + lic[1]).lower()
            else:
                lic_obj = lic
                combined = lic.lower()

            result = _license_is_odbl_compatible(lic_obj)

            if result:
                for term in _mod._INCOMPATIBLE_LICENSE_SUBSTRINGS:
                    self.assertNotIn(
                        term, combined,
                        f"License ALLOWED but contains blocklisted term '{term}': {lic_obj!r}"
                    )
            else:
                matched = any(term in combined for term in _mod._INCOMPATIBLE_LICENSE_SUBSTRINGS)
                self.assertTrue(
                    matched,
                    f"License BLOCKED but no blocklisted term matched: {lic_obj!r}"
                )


class TestIsOdblCompatibleSource(unittest.TestCase):
    def tearDown(self):
        _mod._source_editable_cache.clear()

    def _make_source_json(self, license_info):
        return {
            "layers": {
                "addresses": [
                    {"name": "city", "data": "https://example.com/data", "license": license_info}
                ]
            }
        }

    def _register(self, source_key, source_json):
        _load_source_json_orig = _mod._load_source_json
        _mod._load_source_json = lambda key, oa_sources_dir=None: (
            source_json if key == source_key else _load_source_json_orig(key, oa_sources_dir)
        )
        try:
            return _is_odbl_compatible_source(source_key)
        finally:
            _mod._load_source_json = _load_source_json_orig

    def test_odbl_layer_is_compatible(self):
        source_json = self._make_source_json({"url": "https://opendatacommons.org/licenses/odbl/"})
        self.assertTrue(self._register("test/src", source_json))

    def test_nc_layer_is_blocked(self):
        source_json = self._make_source_json({"url": "https://creativecommons.org/licenses/by-nc/4.0/"})
        self.assertFalse(self._register("test/src_nc", source_json))

    def test_missing_source_json_defaults_to_true(self):
        self.assertTrue(_is_odbl_compatible_source("does/not/exist"))

    def test_no_license_field_defaults_to_true(self):
        source_json = {"layers": {"addresses": [{"name": "city", "data": "https://example.com"}]}}
        self.assertTrue(self._register("test/no_license", source_json))

    def test_result_is_cached(self):
        source_json = self._make_source_json({"url": "https://opendatacommons.org/licenses/odbl/"})
        self.assertTrue(self._register("test/cached", source_json))
        self.assertTrue(_is_odbl_compatible_source("test/cached"))


class TestGetSourceAttribution(unittest.TestCase):
    """Test extraction of attribution strings from OA source JSONs."""

    def _make_source(self, layers):
        return {"layers": {"addresses": layers}}

    def test_license_url_and_text(self):
        src = self._make_source([{
            "name": "city",
            "data": "https://example.com",
            "license": {"url": "https://opendata.vancouver.ca/pages/licence/",
                        "text": "Open Government License - Vancouver 1.0"},
        }])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, text = _get_source_attribution("ca/bc/vancouver")
        self.assertEqual(name, "")
        self.assertEqual(url, "https://opendata.vancouver.ca/pages/licence/")
        self.assertEqual(text, "Open Government License - Vancouver 1.0")

    def test_attribution_field_becomes_source_name(self):
        src = self._make_source([{
            "name": "city",
            "data": "https://example.com",
            "attribution": "US Virgin Islands",
            "license": {"url": "https://creativecommons.org/publicdomain/zero/1.0/"},
        }])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, text = _get_source_attribution("test/key")
        self.assertEqual(name, "US Virgin Islands")
        self.assertEqual(url, "https://creativecommons.org/publicdomain/zero/1.0/")

    def test_website_fallback_when_no_attribution(self):
        src = self._make_source([{
            "name": "city",
            "data": "https://example.com",
            "website": "https://gis-ethekwini.opendata.arcgis.com",
        }])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, text = _get_source_attribution("test/key")
        self.assertEqual(name, "https://gis-ethekwini.opendata.arcgis.com")
        self.assertEqual(url, "")
        self.assertEqual(text, "")

    def test_attribution_priority_over_website(self):
        src = self._make_source([{
            "name": "city",
            "attribution": "VT Enhanced 911 Board",
            "website": "https://vcgi.vermont.gov",
            "license": {"url": "https://creativecommons.org/publicdomain/zero/1.0/"},
        }])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, _ = _get_source_attribution("test/key")
        self.assertEqual(name, "VT Enhanced 911 Board")

    def test_no_license_no_attribution_returns_empty(self):
        src = self._make_source([{"name": "city", "data": "https://example.com"}])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, text = _get_source_attribution("test/key")
        self.assertEqual((name, url, text), ("", "", ""))

    def test_missing_source_json_returns_empty(self):
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=None):
            name, url, text = _get_source_attribution("does/not/exist")
        self.assertEqual((name, url, text), ("", "", ""))

    def test_bare_string_license_with_attribution(self):
        src = self._make_source([{
            "name": "city",
            "attribution": "Servicio de Geomática",
            "license": "https://geoweb.montevideo.gub.uy/",
        }])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, text = _get_source_attribution("test/key")
        self.assertEqual(name, "Servicio de Geomática")
        self.assertEqual(url, "https://geoweb.montevideo.gub.uy/")
        self.assertEqual(text, "")

    def test_attribution_name_in_license_dict(self):
        src = self._make_source([{
            "name": "state",
            "license": {
                "url": "https://open.yukon.ca/open-government-licence-yukon",
                "text": "Open Government Licence - Yukon",
                "attribution name": "Contains information licensed under the Open Government Licence – Yukon",
            },
        }])
        with unittest.mock.patch.object(_mod, "_load_source_json", return_value=src):
            name, url, text = _get_source_attribution("ca/yt/province")
        self.assertEqual(name, "Contains information licensed under the Open Government Licence – Yukon")
        self.assertEqual(url, "https://open.yukon.ca/open-government-licence-yukon")
        self.assertEqual(text, "Open Government Licence - Yukon")


class TestProcess(unittest.TestCase):
    """Test process() — TSV output from a GeoJSON ZIP."""

    def _make_geojson_feature(self, lon, lat, number, street, postcode=""):
        return {
            "type": "Feature",
            "properties": {"number": number, "street": street, "postcode": postcode},
            "geometry": {"type": "Point", "coordinates": [lon, lat]},
        }

    def _make_zip(self, geojson_name, features):
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            geojson_content = "\n".join(json.dumps(f) for f in features) + "\n"
            zf.writestr(geojson_name, geojson_content)
        buf.seek(0)
        return buf

    def _run(self, zip_buf):
        out = io.StringIO()
        with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tmp:
            tmp.write(zip_buf.read())
            tmp_path = tmp.name
        try:
            _process(tmp_path, out_file=out)
        finally:
            os.unlink(tmp_path)
        return out.getvalue().rstrip("\n").split("\n")

    def test_single_feature_produces_tsv(self):
        features = [self._make_geojson_feature(-123.12, 49.28, "123", "Main St", "V6B 1A1")]
        lines = self._run(self._make_zip("ca/countrywide-addresses.geojson", features))

        self.assertEqual(len(lines), 1)
        fields = lines[0].split("\t")
        self.assertEqual(len(fields), 9)
        self.assertAlmostEqual(float(fields[0]), 49.28, places=2)
        self.assertAlmostEqual(float(fields[1]), -123.12, places=2)
        self.assertEqual(fields[2], "123")
        self.assertEqual(fields[3], "Main St")
        self.assertEqual(fields[4], "V6B 1A1")
        self.assertIn(fields[5], ("0", "1"))
        # Attribution columns (6-8) may be empty for unresolved sources

    def test_incomplete_feature_skipped(self):
        features = [{"type": "Feature", "properties": {"number": "123"},
                     "geometry": {"type": "Point", "coordinates": [-123.12, 49.28]}}]
        lines = self._run(self._make_zip("ca/countrywide-addresses.geojson", features))
        self.assertEqual(lines, [""])

    def test_multiple_features_same_tsv(self):
        features = [
            self._make_geojson_feature(-123.12, 49.28, "123", "Main St"),
            self._make_geojson_feature(-123.13, 49.29, "456", "Oak Ave", "V6B 2B2"),
        ]
        lines = self._run(self._make_zip("ca/countrywide-addresses.geojson", features))
        self.assertEqual(len(lines), 2)
        numbers = [line.split("\t")[2] for line in lines]
        self.assertEqual(numbers, ["123", "456"])

    def test_duplicate_address_dropped(self):
        features = [
            self._make_geojson_feature(-123.12, 49.28, "123", "Main St"),
            self._make_geojson_feature(-123.13, 49.29, "123", "Main St"),
        ]
        lines = self._run(self._make_zip("ca/countrywide-addresses.geojson", features))
        self.assertEqual(len(lines), 1)

    def test_editable_flag_set(self):
        features = [self._make_geojson_feature(-123.12, 49.28, "123", "Main St")]
        lines = self._run(self._make_zip("ca/countrywide-addresses.geojson", features))
        self.assertEqual(lines[0].split("\t")[5], "1")


if __name__ == "__main__":
    unittest.main()
