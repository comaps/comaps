import importlib.util
import io
import math
import os
import struct
import unittest

_HERE = os.path.dirname(__file__)
_PREPROCESSOR = os.path.normpath(
    os.path.join(_HERE, "..", "generator", "openaddresses_preprocessor.py")
)
_spec = importlib.util.spec_from_file_location("openaddresses_preprocessor", _PREPROCESSOR)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)

_double_to_uint32 = _mod._double_to_uint32
_lat_to_y         = _mod._lat_to_y
_perfect_shuffle  = _mod._perfect_shuffle
_point_to_int64   = _mod._point_to_int64
_write_string     = _mod._write_string
_write_varint     = _mod._write_varint
_write_varuint    = _mod._write_varuint
_zigzag_encode    = _mod._zigzag_encode
_COORD_BITS       = _mod._COORD_BITS
_COORD_SIZE       = _mod._COORD_SIZE
_MIN_X            = _mod._MIN_X
_MAX_X            = _mod._MAX_X
_MIN_Y            = _mod._MIN_Y
_MAX_Y            = _mod._MAX_Y



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
        ys = [_lat_to_y(l) for l in lats]
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

    def _build_record(self, number, street, postcode, lon, lat):
        f = io.BytesIO()
        _write_string(f, number)
        _write_string(f, number)
        _write_string(f, street)
        _write_string(f, postcode)
        f.write(bytes([_mod._INTERPOL_NONE]))
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
        interpol = data[pos[0]]; pos[0] += 1
        count = read_varuint()
        point = read_varint()
        return {
            "m_from": m_from, "m_to": m_to, "m_street": m_street,
            "m_postcode": m_postcode, "interpol": interpol,
            "count": count, "point": point,
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


if __name__ == "__main__":
    unittest.main()
