import importlib.util
import io
import json
import os
import tempfile
import textwrap
import unittest
import zipfile

_HERE = os.path.dirname(__file__)
_PREPROCESSOR = os.path.normpath(
    os.path.join(_HERE, "..", "generator", "openaddresses_preprocessor.py")
)
_spec = importlib.util.spec_from_file_location("openaddresses_preprocessor", _PREPROCESSOR)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)

_double_to_uint32           = _mod._double_to_uint32
_lat_to_y                   = _mod._lat_to_y
_perfect_shuffle             = _mod._perfect_shuffle
_point_to_int64             = _mod._point_to_int64
_write_string               = _mod._write_string
_write_varint               = _mod._write_varint
_write_varuint              = _mod._write_varuint
_zigzag_encode              = _mod._zigzag_encode
_find_address_geojsons      = _mod._find_address_geojsons
_parse_poly                 = _mod._parse_poly
_BorderIndex                = _mod._BorderIndex
_COORD_BITS                 = _mod._COORD_BITS
_COORD_SIZE                 = _mod._COORD_SIZE
_MIN_X                      = _mod._MIN_X
_MAX_X                      = _mod._MAX_X
_MIN_Y                      = _mod._MIN_Y
_MAX_Y                      = _mod._MAX_Y
_license_is_odbl_compatible = _mod._license_is_odbl_compatible


class TestWriteVaruint(unittest.TestCase):
    def _encode(self, value):
        f = io.BytesIO()
        _write_varuint(f, value)
        return f.getvalue()

    def test_zero(self):
        self.assertEqual(self._encode(0), b'\x00')

    def test_one(self):
        self.assertEqual(self._encode(1), b'\x01')

    def test_127(self):
        self.assertEqual(self._encode(127), b'\x7f')

    def test_128(self):
        self.assertEqual(self._encode(128), b'\x80\x01')

    def test_300(self):
        self.assertEqual(self._encode(300), b'\xac\x02')

    def test_16383(self):
        self.assertEqual(self._encode(16383), b'\xff\x7f')

    def test_16384(self):
        self.assertEqual(self._encode(16384), b'\x80\x80\x01')



class TestZigzagEncode(unittest.TestCase):
    def test_zero(self):
        self.assertEqual(_zigzag_encode(0), 0)

    def test_minus_one(self):
        self.assertEqual(_zigzag_encode(-1), 1)

    def test_one(self):
        self.assertEqual(_zigzag_encode(1), 2)

    def test_minus_two(self):
        self.assertEqual(_zigzag_encode(-2), 3)

    def test_large_positive(self):
        self.assertEqual(_zigzag_encode(2147483647), 4294967294)

    def test_large_negative(self):
        self.assertEqual(_zigzag_encode(-2147483648), 4294967295)



class TestWriteVarint(unittest.TestCase):
    def _encode(self, value):
        f = io.BytesIO()
        _write_varint(f, value)
        return f.getvalue()

    def test_zero(self):
        self.assertEqual(self._encode(0), b'\x00')

    def test_minus_one(self):
        self.assertEqual(self._encode(-1), b'\x01')

    def test_one(self):
        self.assertEqual(self._encode(1), b'\x02')

    def test_minus_64(self):
        self.assertEqual(self._encode(-64), b'\x7f')

    def test_64(self):
        self.assertEqual(self._encode(64), b'\x80\x01')



class TestWriteString(unittest.TestCase):
    def _encode(self, s):
        f = io.BytesIO()
        _write_string(f, s)
        return f.getvalue()

    def test_empty(self):
        self.assertEqual(self._encode(""), b'\x00')

    def test_ascii(self):
        data = self._encode("42")
        self.assertEqual(data, b'\x02' + b'42')

    def test_ascii_street(self):
        data = self._encode("Main St")
        self.assertEqual(data, b'\x07' + b'Main St')

    def test_utf8(self):
        encoded = "café".encode("utf-8")
        data = self._encode("café")
        self.assertEqual(data, bytes([len(encoded)]) + encoded)

    def test_length_127(self):
        s = "x" * 127
        data = self._encode(s)
        self.assertEqual(data[0:1], b'\x7f')
        self.assertEqual(data[1:], s.encode())

    def test_length_128(self):
        s = "x" * 128
        data = self._encode(s)
        self.assertEqual(data[0:2], b'\x80\x01')
        self.assertEqual(data[2:], s.encode())



class TestLatToY(unittest.TestCase):
    def test_equator(self):
        self.assertAlmostEqual(_lat_to_y(0.0), 0.0, places=10)

    def test_symmetry(self):
        for lat in [10.0, 45.0, 60.0, 80.0]:
            self.assertAlmostEqual(_lat_to_y(-lat), -_lat_to_y(lat), places=10)

    def test_monotone(self):
        lats = [0, 10, 30, 49, 60, 80]
        ys = [_lat_to_y(lat_val) for lat_val in lats]
        for i in range(len(ys) - 1):
            self.assertLess(ys[i], ys[i + 1])

    def test_clamped_min(self):
        self.assertEqual(_lat_to_y(-90.0), _MIN_Y)

    def test_clamped_max(self):
        self.assertEqual(_lat_to_y(90.0), _MAX_Y)

    def test_known_value_45(self):
        y = _lat_to_y(45.0)
        self.assertGreater(y, 49.0)
        self.assertLess(y, 52.0)

    def test_vancouver(self):
        y = _lat_to_y(49.25)
        self.assertGreater(y, 45.0)
        self.assertLess(y, 60.0)



class TestDoubleToUint32(unittest.TestCase):
    def test_min_boundary(self):
        self.assertEqual(_double_to_uint32(_MIN_X, _MIN_X, _MAX_X), 0)

    def test_max_boundary(self):
        self.assertEqual(_double_to_uint32(_MAX_X, _MIN_X, _MAX_X), int(_COORD_SIZE))

    def test_below_min_clamps(self):
        self.assertEqual(_double_to_uint32(-200.0, _MIN_X, _MAX_X), 0)

    def test_above_max_clamps(self):
        self.assertEqual(_double_to_uint32(200.0, _MIN_X, _MAX_X), int(_COORD_SIZE))

    def test_center(self):
        mid = _double_to_uint32(0.0, _MIN_X, _MAX_X)
        expected = round(_COORD_SIZE / 2)
        self.assertAlmostEqual(mid, expected, delta=1)

    def test_result_in_range(self):
        for lon in [-180, -90, 0, 49.25, 90, 180]:
            val = _double_to_uint32(lon, _MIN_X, _MAX_X)
            self.assertGreaterEqual(val, 0)
            self.assertLessEqual(val, int(_COORD_SIZE))



class TestPerfectShuffle(unittest.TestCase):
    def test_zero(self):
        self.assertEqual(_perfect_shuffle(0), 0)

    def test_low_32_bit0(self):
        self.assertEqual(_perfect_shuffle(1) & 1, 1)

    def test_high_32_bit0(self):
        self.assertEqual((_perfect_shuffle(1 << 32) >> 1) & 1, 1)

    def test_output_fits_64bits(self):
        self.assertEqual(_perfect_shuffle(0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF)



class TestPointToInt64(unittest.TestCase):
    def test_result_is_nonnegative(self):
        for lon, lat in [(0, 0), (-123.1, 49.25), (180, 86), (-180, -86)]:
            result = _point_to_int64(lon, lat)
            self.assertGreaterEqual(result, 0)

    def test_real_coords_nonzero(self):
        for lon, lat in [(-123.1, 49.25), (-75.7, 45.4), (-114.1, 51.0)]:
            self.assertGreater(_point_to_int64(lon, lat), 0)

    def test_deterministic(self):
        p1 = _point_to_int64(-123.1, 49.25)
        p2 = _point_to_int64(-123.1, 49.25)
        self.assertEqual(p1, p2)

    def test_different_coords_differ(self):
        p1 = _point_to_int64(-123.1, 49.25)
        p2 = _point_to_int64(-75.7, 45.4)
        self.assertNotEqual(p1, p2)

    def test_fits_in_int64(self):
        v = _point_to_int64(-123.1, 49.25)
        self.assertLessEqual(v, 0x7FFFFFFFFFFFFFFF)


class TestRecordRoundTrip(unittest.TestCase):

    def _build_record(self, number, street, postcode, lon, lat, editable=True):
        f = io.BytesIO()
        _write_string(f, number)
        _write_string(f, number)
        _write_string(f, street)
        _write_string(f, postcode)
        f.write(bytes([_mod._INTERPOL_NONE]))
        f.write(bytes([1 if editable else 0]))
        _write_varuint(f, 1)
        _write_varint(f, _point_to_int64(lon, lat))
        return f.getvalue()

    def _parse_record(self, data):
        pos = [0]

        def read_varuint():
            result = 0
            shift = 0
            while True:
                b = data[pos[0]]
                pos[0] += 1
                result |= (b & 0x7F) << shift
                if not (b & 0x80):
                    break
                shift += 7
            return result

        def read_varint():
            v = read_varuint()
            return (v >> 1) ^ -(v & 1)

        def read_string():
            length = read_varuint()
            s = data[pos[0]:pos[0] + length].decode("utf-8")
            pos[0] += length
            return s

        m_from = read_string()
        m_to = read_string()
        m_street = read_string()
        m_postcode = read_string()
        interpol = data[pos[0]]
        pos[0] += 1
        editable = data[pos[0]] != 0
        pos[0] += 1
        count = read_varuint()
        point = read_varint()
        return {
            "m_from": m_from, "m_to": m_to, "m_street": m_street,
            "m_postcode": m_postcode, "interpol": interpol,
            "editable": editable, "count": count, "point": point,
        }

    def test_from_equals_to(self):
        data = self._build_record("42", "Main St", "V5K 0A1", -123.1, 49.25)
        rec = self._parse_record(data)
        self.assertEqual(rec["m_from"], "42")
        self.assertEqual(rec["m_to"], "42")
        self.assertEqual(rec["m_from"], rec["m_to"])

    def test_street_and_postcode(self):
        data = self._build_record("100", "Oak Ave", "V6B 2W2", -123.0, 49.3)
        rec = self._parse_record(data)
        self.assertEqual(rec["m_street"], "Oak Ave")
        self.assertEqual(rec["m_postcode"], "V6B 2W2")

    def test_interpol_none(self):
        data = self._build_record("1", "A St", "", -123.0, 49.0)
        rec = self._parse_record(data)
        self.assertEqual(rec["interpol"], 0)

    def test_point_count_one(self):
        data = self._build_record("1", "A St", "", -123.0, 49.0)
        rec = self._parse_record(data)
        self.assertEqual(rec["count"], 1)

    def test_point_round_trips(self):
        lon, lat = -123.1, 49.25
        expected = _point_to_int64(lon, lat)
        data = self._build_record("1", "A", "", lon, lat)
        rec = self._parse_record(data)
        self.assertEqual(rec["point"], expected)

    def test_empty_postcode(self):
        data = self._build_record("5", "Elm Rd", "", -75.7, 45.4)
        rec = self._parse_record(data)
        self.assertEqual(rec["m_postcode"], "")

    def test_record_fully_consumed(self):
        data = self._build_record("99", "King St", "K1A 0A6", -75.7, 45.4)
        rec = self._parse_record(data)
        self.assertEqual(rec["m_from"], "99")

    def test_editable_true(self):
        data = self._build_record("1", "A St", "", -123.0, 49.0, editable=True)
        rec = self._parse_record(data)
        self.assertTrue(rec["editable"])

    def test_editable_false(self):
        data = self._build_record("1", "A St", "", -123.0, 49.0, editable=False)
        rec = self._parse_record(data)
        self.assertFalse(rec["editable"])


def _write_poly(directory: str, name: str, rings: list) -> str:
    """Write a minimal .poly file and return its path.

    rings is a list of lists of (lon, lat) tuples.  The first ring is outer;
    subsequent rings are holes (prefixed with '!').
    """
    path = os.path.join(directory, name + ".poly")
    with open(path, "w") as fh:
        fh.write(name + "\n")
        for i, ring in enumerate(rings):
            prefix = "!" if i > 0 else ""
            fh.write(f"{prefix}{i + 1}\n")
            for lon, lat in ring:
                fh.write(f"   {lon:.6E}   {lat:.6E}\n")
            fh.write("END\n")
        fh.write("END\n")
    return path


_SQUARE = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]


class TestFindAddressGeojsons(unittest.TestCase):
    def _make_zip(self, names):
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            for name in names:
                zf.writestr(name, b"")
        buf.seek(0)
        return zipfile.ZipFile(buf, "r")

    # --- Canada-style countrywide files ---

    def test_finds_canada_countrywide(self):
        zf = self._make_zip(["ca/countrywide-addresses-country.geojson", "ca/other.geojson"])
        result = _find_address_geojsons(zf)
        self.assertEqual(result, ["ca/countrywide-addresses-country.geojson"])
        zf.close()

    def test_finds_multiple_countrywide(self):
        zf = self._make_zip(["ca/countrywide.geojson", "au/countrywide.geojson", "ca/other.csv"])
        result = _find_address_geojsons(zf)
        self.assertIn("ca/countrywide.geojson", result)
        self.assertIn("au/countrywide.geojson", result)
        self.assertEqual(len(result), 2)
        zf.close()

    # --- US-style county/state address files ---

    def test_finds_us_county_addresses(self):
        zf = self._make_zip([
            "us/or/marion-addresses-county.geojson",
            "us/or/marion-parcels-county.geojson",
            "us/or/marion-parcels-county.geojson.meta",
        ])
        result = _find_address_geojsons(zf)
        self.assertEqual(result, ["us/or/marion-addresses-county.geojson"])
        zf.close()

    def test_finds_us_statewide_addresses(self):
        zf = self._make_zip([
            "us/or/statewide-addresses-state.geojson",
            "us/or/statewide-buildings-state.geojson",
        ])
        result = _find_address_geojsons(zf)
        self.assertEqual(result, ["us/or/statewide-addresses-state.geojson"])
        zf.close()

    def test_finds_multiple_us_counties(self):
        zf = self._make_zip([
            "us/or/marion-addresses-county.geojson",
            "us/or/yamhill-addresses-county.geojson",
            "us/wa/king-addresses-county.geojson",
            "us/or/marion-parcels-county.geojson",
        ])
        result = _find_address_geojsons(zf)
        self.assertIn("us/or/marion-addresses-county.geojson", result)
        self.assertIn("us/or/yamhill-addresses-county.geojson", result)
        self.assertIn("us/wa/king-addresses-county.geojson", result)
        self.assertEqual(len(result), 3)
        zf.close()

    def test_finds_depth2_addresses_without_countrywide(self):
        # e.g. mx/national-addresses-country.geojson — depth 2, no "countrywide"
        zf = self._make_zip(["mx/national-addresses-country.geojson", "mx/national-buildings-country.geojson"])
        result = _find_address_geojsons(zf)
        self.assertEqual(result, ["mx/national-addresses-country.geojson"])
        zf.close()

    def test_ignores_meta_files(self):
        zf = self._make_zip([
            "us/or/marion-addresses-county.geojson",
            "us/or/marion-addresses-county.geojson.meta",
        ])
        result = _find_address_geojsons(zf)
        self.assertEqual(result, ["us/or/marion-addresses-county.geojson"])
        zf.close()

    def test_ignores_non_address_geojsons(self):
        zf = self._make_zip([
            "us/or/marion-parcels-county.geojson",
            "us/or/statewide-buildings-state.geojson",
        ])
        with self.assertRaises(ValueError):
            _find_address_geojsons(zf)
        zf.close()

    def test_raises_when_empty_zip(self):
        zf = self._make_zip(["ca/other.csv", "readme.txt"])
        with self.assertRaises(ValueError):
            _find_address_geojsons(zf)
        zf.close()

    def test_ignores_non_geojson_extension(self):
        zf = self._make_zip(["ca/countrywide.csv", "ca/countrywide.zip"])
        with self.assertRaises(ValueError):
            _find_address_geojsons(zf)
        zf.close()


class TestParsePoly(unittest.TestCase):
    def _write(self, content: str) -> str:
        fd, path = tempfile.mkstemp(suffix=".poly")
        os.write(fd, content.encode())
        os.close(fd)
        return path

    def test_single_ring(self):
        content = textwrap.dedent("""\
            TestRegion
            1
               0.0   0.0
               1.0   0.0
               1.0   1.0
               0.0   1.0
            END
            END
        """)
        path = self._write(content)
        rings = _parse_poly(path)
        os.unlink(path)
        self.assertEqual(len(rings), 1)
        self.assertEqual(len(rings[0]), 4)
        self.assertAlmostEqual(rings[0][0][0], 0.0)
        self.assertAlmostEqual(rings[0][0][1], 0.0)

    def test_scientific_notation(self):
        content = textwrap.dedent("""\
            TestRegion
            1
               -1.208514E+02   4.900030E+01
               -1.208608E+02   4.900030E+01
            END
            END
        """)
        path = self._write(content)
        rings = _parse_poly(path)
        os.unlink(path)
        self.assertEqual(len(rings), 1)
        self.assertAlmostEqual(rings[0][0][0], -120.8514, places=3)
        self.assertAlmostEqual(rings[0][0][1], 49.0003, places=3)

    def test_multiple_rings(self):
        content = textwrap.dedent("""\
            TestRegion
            1
               0.0   0.0
               1.0   0.0
               1.0   1.0
            END
            !2
               0.2   0.2
               0.8   0.2
               0.8   0.8
            END
            END
        """)
        path = self._write(content)
        rings = _parse_poly(path)
        os.unlink(path)
        self.assertEqual(len(rings), 2)
        self.assertEqual(len(rings[0]), 3)
        self.assertEqual(len(rings[1]), 3)

    def test_empty_file_returns_no_rings(self):
        content = "TestRegion\nEND\n"
        path = self._write(content)
        rings = _parse_poly(path)
        os.unlink(path)
        self.assertEqual(rings, [])


class TestBorderIndex(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        # Region A: unit square (0,0)-(1,1)
        _write_poly(self.tmpdir, "RegionA", [_SQUARE])
        # Region B: unit square (2,0)-(3,1)
        _write_poly(self.tmpdir, "RegionB", [[(2.0, 0.0), (3.0, 0.0), (3.0, 1.0), (2.0, 1.0)]])

    def tearDown(self):
        import shutil
        shutil.rmtree(self.tmpdir)

    def test_point_in_region_a(self):
        idx = _BorderIndex(self.tmpdir)
        self.assertEqual(idx.find(0.5, 0.5), "RegionA")

    def test_point_in_region_b(self):
        idx = _BorderIndex(self.tmpdir)
        self.assertEqual(idx.find(2.5, 0.5), "RegionB")

    def test_point_outside_all(self):
        idx = _BorderIndex(self.tmpdir)
        self.assertIsNone(idx.find(5.0, 5.0))

    def test_point_between_regions(self):
        idx = _BorderIndex(self.tmpdir)
        self.assertIsNone(idx.find(1.5, 0.5))

    def test_no_poly_files_raises(self):
        empty = tempfile.mkdtemp()
        try:
            with self.assertRaises(ValueError):
                _BorderIndex(empty)
        finally:
            os.rmdir(empty)

    def test_mwm_name_strips_poly_extension(self):
        idx = _BorderIndex(self.tmpdir)
        result = idx.find(0.5, 0.5)
        self.assertFalse(result.endswith(".poly"))


class TestProcess(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.mkdtemp()
        self.borders = tempfile.mkdtemp()
        self.output = tempfile.mkdtemp()
        _write_poly(self.borders, "TestRegion", [_SQUARE])

    def tearDown(self):
        import shutil
        shutil.rmtree(self.tmpdir)
        shutil.rmtree(self.borders)
        shutil.rmtree(self.output)

    def _make_zip(self, features):
        path = os.path.join(self.tmpdir, "test.zip")
        lines = "\n".join(json.dumps(f) for f in features)
        with zipfile.ZipFile(path, "w") as zf:
            zf.writestr("cc/countrywide.geojson", lines)
        return path

    def _feat(self, lon, lat, number="42", street="Main St", postcode="V5K 0A1"):
        return {
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [lon, lat]},
            "properties": {"number": number, "street": street, "postcode": postcode},
        }

    def _read_records(self, path):
        with open(path, "rb") as fh:
            data = fh.read()
        pos = [0]
        records = []

        def read_varuint():
            result = 0
            shift = 0
            while True:
                b = data[pos[0]]
                pos[0] += 1
                result |= (b & 0x7F) << shift
                if not (b & 0x80):
                    break
                shift += 7
            return result

        def read_varint():
            v = read_varuint()
            return (v >> 1) ^ -(v & 1)

        def read_string():
            n = read_varuint()
            s = data[pos[0]:pos[0] + n].decode("utf-8")
            pos[0] += n
            return s

        while pos[0] < len(data):
            m_from = read_string()
            m_to = read_string()
            m_street = read_string()
            m_postcode = read_string()
            interpol = data[pos[0]]
            pos[0] += 1
            editable = data[pos[0]] != 0
            pos[0] += 1
            count = read_varuint()
            point = read_varint()
            records.append({
                "m_from": m_from, "m_to": m_to, "m_street": m_street,
                "m_postcode": m_postcode, "interpol": interpol,
                "editable": editable, "count": count, "point": point,
            })
        return records

    def test_valid_feature_produces_record(self):
        zip_path = self._make_zip([self._feat(0.5, 0.5)])
        _mod.process(zip_path, self.output, self.borders)
        out_file = os.path.join(self.output, "TestRegion.tempaddr")
        self.assertTrue(os.path.exists(out_file))
        records = self._read_records(out_file)
        self.assertEqual(len(records), 1)
        r = records[0]
        self.assertEqual(r["m_from"], "42")
        self.assertEqual(r["m_to"], "42")
        self.assertEqual(r["m_street"], "Main St")
        self.assertEqual(r["m_postcode"], "V5K 0A1")
        self.assertEqual(r["interpol"], 0)
        self.assertEqual(r["count"], 1)
        self.assertEqual(r["point"], _point_to_int64(0.5, 0.5))

    def test_feature_outside_region_not_written(self):
        zip_path = self._make_zip([self._feat(5.0, 5.0)])
        _mod.process(zip_path, self.output, self.borders)
        self.assertEqual(os.listdir(self.output), [])

    def test_incomplete_feature_skipped(self):
        zip_path = self._make_zip([
            self._feat(0.5, 0.5, number=""),
            self._feat(0.5, 0.5, street=""),
        ])
        _mod.process(zip_path, self.output, self.borders)
        self.assertEqual(os.listdir(self.output), [])

    def test_multiple_features_same_region(self):
        zip_path = self._make_zip([
            self._feat(0.2, 0.2, number="1", street="A St"),
            self._feat(0.8, 0.8, number="2", street="B Ave"),
        ])
        _mod.process(zip_path, self.output, self.borders)
        out_file = os.path.join(self.output, "TestRegion.tempaddr")
        records = self._read_records(out_file)
        self.assertEqual(len(records), 2)
        streets = {r["m_street"] for r in records}
        self.assertEqual(streets, {"A St", "B Ave"})

    def test_mixed_valid_and_invalid(self):
        zip_path = self._make_zip([
            self._feat(0.5, 0.5),
            self._feat(5.0, 5.0),
            self._feat(0.3, 0.3, number=""),
        ])
        _mod.process(zip_path, self.output, self.borders)
        out_file = os.path.join(self.output, "TestRegion.tempaddr")
        records = self._read_records(out_file)
        self.assertEqual(len(records), 1)


class TestLicenseIsOdblCompatible(unittest.TestCase):
    def _check(self, license_info):
        return _mod._license_is_odbl_compatible(license_info)

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
        """Walk OA sources/ tree and collect unique license values."""
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

    # Licenses known to be ODbL-compatible. The blacklist must NOT block these.
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
        # Municipal Open Government Licences
        "https://opendata.vancouver.ca/pages/licence/",
        "https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/",
        "https://data.edmonton.ca/stories/s/City-of-Edmonton-Open-Data-Terms-of-Use/msh8-if28/",
    }

    # Licenses known to be ODbL-incompatible. The blacklist MUST block these.
    KNOWN_INCOMPATIBLE = {
        "https://creativecommons.org/licenses/by-nc/4.0/",
        "https://creativecommons.org/licenses/by-nd/4.0/",
        "https://creativecommons.org/licenses/by-nc-sa/4.0/",
        "https://creativecommons.org/licenses/by-sa/4.0/",
        "https://creativecommons.org/licenses/by-sa/3.0/",
    }

    def test_known_compatible_not_blocked(self):
        """No known-good license should be rejected by the blacklist."""
        for lic_url in self.KNOWN_COMPATIBLE:
            with self.subTest(license=lic_url):
                self.assertTrue(
                    _license_is_odbl_compatible({"url": lic_url}),
                    f"Known-compatible license was BLOCKED: {lic_url}"
                )

    def test_known_incompatible_are_blocked(self):
        """Known-bad licenses must be rejected by the blacklist."""
        for lic_url in self.KNOWN_INCOMPATIBLE:
            with self.subTest(license=lic_url):
                self.assertFalse(
                    _license_is_odbl_compatible({"url": lic_url}),
                    f"Known-incompatible license was ALLOWED: {lic_url}"
                )

    @unittest.skipUnless(os.environ.get("OA_SOURCES_DIR"), "Set OA_SOURCES_DIR to run full license audit")
    def test_all_real_licenses_no_false_positives(self):
        """Every real OA license is either marked compatible or there's a reason it's not.

        This catches regressions where a blacklist term is too broad and
        accidentally blocks good licenses (e.g. ``-sa-`` matching a legitimate URL).
        """
        oa_dir = os.environ["OA_SOURCES_DIR"]
        licenses = self._collect_real_licenses(oa_dir)

        # Licenses we explicitly expect to be blocked (and why).
        blocked_expected = {
            "https://creativecommons.org/licenses/by-nc-sa/4.0/": "-nc-",
            "https://creativecommons.org/licenses/by-nc/4.0/": None,  # excluded from check below
        }

        for lic in licenses:
            if isinstance(lic, tuple):
                lic_obj = {"url": lic[0], "text": lic[1]}
                combined = (lic[0] + " " + lic[1]).lower()
            else:
                lic_obj = lic
                combined = lic.lower()

            result = _license_is_odbl_compatible(lic_obj)

            if result:
                # Allowed — must NOT contain any blocklisted substrings.
                for term in _mod._INCOMPATIBLE_LICENSE_SUBSTRINGS:
                    self.assertNotIn(
                        term, combined,
                        f"License ALLOWED but contains blocklisted term '{term}': {lic_obj!r}"
                    )
            else:
                # Blocked — must contain at least one blocklisted substring.
                matched = any(term in combined for term in _mod._INCOMPATIBLE_LICENSE_SUBSTRINGS)
                self.assertTrue(
                    matched,
                    f"License BLOCKED but no blocklisted term matched: {lic_obj!r}"
                )


class TestSourceKeyFromGeojsonPath(unittest.TestCase):
    def _key(self, path):
        return _mod._source_key_from_geojson_path(path)

    def test_county_path(self):
        self.assertEqual(
            self._key("ca/bc/city_of_victoria-addresses-county.geojson"),
            "ca/bc/city_of_victoria",
        )

    def test_city_path(self):
        self.assertEqual(
            self._key("us/wa/city_of_spokane-addresses-city.geojson"),
            "us/wa/city_of_spokane",
        )

    def test_countrywide_path(self):
        self.assertEqual(
            self._key("ca/countrywide.geojson"),
            "ca/countrywide",
        )

    def test_no_layer_suffix(self):
        self.assertEqual(
            self._key("us/or/portland.geojson"),
            "us/or/portland",
        )


class TestIsOdblCompatibleSource(unittest.TestCase):
    def setUp(self):
        # Isolate cache between tests.
        _mod._source_editable_cache.clear()

    def tearDown(self):
        _mod._source_editable_cache.clear()

    def _mock_load(self, source_json):
        """Patch _load_source_json to return source_json without HTTP."""
        import unittest.mock
        return unittest.mock.patch.object(_mod, "_load_source_json", return_value=source_json)

    def test_odbl_layer_is_compatible(self):
        source = {
            "layers": {
                "addresses": [
                    {"license": {"url": "https://opendatacommons.org/licenses/odbl/1.0/"}}
                ]
            }
        }
        with self._mock_load(source):
            self.assertTrue(_mod._is_odbl_compatible_source("ca/bc/test"))

    def test_nc_layer_is_blocked(self):
        source = {
            "layers": {
                "addresses": [
                    {"license": {"url": "https://creativecommons.org/licenses/by-nc/4.0/"}}
                ]
            }
        }
        with self._mock_load(source):
            self.assertFalse(_mod._is_odbl_compatible_source("ca/bc/nc_source"))

    def test_missing_source_json_defaults_to_true(self):
        with self._mock_load(None):
            self.assertTrue(_mod._is_odbl_compatible_source("ca/bc/missing"))

    def test_no_license_field_defaults_to_true(self):
        source = {"layers": {"addresses": [{"data": "something"}]}}
        with self._mock_load(source):
            self.assertTrue(_mod._is_odbl_compatible_source("ca/bc/nolicense"))

    def test_result_is_cached(self):
        source = {
            "layers": {
                "addresses": [
                    {"license": {"url": "https://creativecommons.org/licenses/by-nc/4.0/"}}
                ]
            }
        }
        import unittest.mock
        mock_fn = unittest.mock.MagicMock(return_value=source)
        with unittest.mock.patch.object(_mod, "_load_source_json", mock_fn):
            _mod._is_odbl_compatible_source("ca/bc/cached")
            _mod._is_odbl_compatible_source("ca/bc/cached")
        self.assertEqual(mock_fn.call_count, 1)

    def test_local_sources_dir_reads_from_disk(self):
        source = {
            "layers": {
                "addresses": [
                    {"license": {"url": "https://creativecommons.org/licenses/by-nc/4.0/"}}
                ]
            }
        }
        with tempfile.TemporaryDirectory() as d:
            source_path = os.path.join(d, "sources", "ca", "bc")
            os.makedirs(source_path)
            with open(os.path.join(source_path, "test.json"), "w") as f:
                json.dump(source, f)
            self.assertFalse(_mod._is_odbl_compatible_source("ca/bc/test", d))

    def test_local_sources_dir_missing_file_defaults_to_true(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertTrue(_mod._is_odbl_compatible_source("ca/bc/nonexistent", d))


if __name__ == "__main__":
    unittest.main()
