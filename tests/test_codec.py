"""Tests for the binary codec (PacketReader / PacketWriter)."""

import struct


from endstone_endweave.codec import PacketReader
from endstone_endweave.codec.writer import PacketWriter


class TestVarint:
    def test_roundtrip_zero(self):
        w = PacketWriter()
        w.write_uvarint(0)
        r = PacketReader(w.to_bytes())
        assert r.read_uvarint() == 0

    def test_roundtrip_small(self):
        for val in [1, 42, 127]:
            w = PacketWriter()
            w.write_uvarint(val)
            r = PacketReader(w.to_bytes())
            assert r.read_uvarint() == val

    def test_roundtrip_multibyte(self):
        for val in [128, 300, 16384, 2097152, 0xFFFFFFFF]:
            w = PacketWriter()
            w.write_uvarint(val)
            r = PacketReader(w.to_bytes())
            assert r.read_uvarint() == val

    def test_single_byte_encoding(self):
        w = PacketWriter()
        w.write_uvarint(1)
        assert w.to_bytes() == b"\x01"

    def test_two_byte_encoding(self):
        w = PacketWriter()
        w.write_uvarint(300)
        data = w.to_bytes()
        assert len(data) == 2
        r = PacketReader(data)
        assert r.read_uvarint() == 300


class TestSignedVarint:
    def test_roundtrip(self):
        for val in [0, 1, -1, 42, -42, 2147483647, -2147483648]:
            w = PacketWriter()
            w.write_varint(val)
            r = PacketReader(w.to_bytes())
            assert r.read_varint() == val

    def test_zigzag_encoding(self):
        # zigzag: 0→0, -1→1, 1→2, -2→3
        w = PacketWriter()
        w.write_varint(0)
        assert w.to_bytes() == b"\x00"

        w = PacketWriter()
        w.write_varint(-1)
        assert w.to_bytes() == b"\x01"

        w = PacketWriter()
        w.write_varint(1)
        assert w.to_bytes() == b"\x02"


class TestVarlong:
    def test_roundtrip(self):
        for val in [0, 1, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF]:
            w = PacketWriter()
            w.write_uvarint64(val)
            r = PacketReader(w.to_bytes())
            assert r.read_uvarint64() == val


class TestString:
    def test_roundtrip_empty(self):
        w = PacketWriter()
        w.write_string("")
        r = PacketReader(w.to_bytes())
        assert r.read_string() == ""

    def test_roundtrip_ascii(self):
        w = PacketWriter()
        w.write_string("hello")
        r = PacketReader(w.to_bytes())
        assert r.read_string() == "hello"

    def test_roundtrip_unicode(self):
        w = PacketWriter()
        w.write_string("hello 🌍")
        r = PacketReader(w.to_bytes())
        assert r.read_string() == "hello 🌍"

    def test_length_prefix(self):
        w = PacketWriter()
        w.write_string("abc")
        data = w.to_bytes()
        assert data[0] == 3  # length prefix
        assert data[1:] == b"abc"


class TestPrimitives:
    def test_byte(self):
        w = PacketWriter()
        w.write_byte(0xFF)
        r = PacketReader(w.to_bytes())
        assert r.read_byte() == 0xFF

    def test_bool(self):
        w = PacketWriter()
        w.write_bool(True)
        w.write_bool(False)
        r = PacketReader(w.to_bytes())
        assert r.read_bool() is True
        assert r.read_bool() is False

    def test_short_le(self):
        w = PacketWriter()
        w.write_short_le(-1234)
        r = PacketReader(w.to_bytes())
        assert r.read_short_le() == -1234

    def test_int_le(self):
        w = PacketWriter()
        w.write_int_le(-100000)
        r = PacketReader(w.to_bytes())
        assert r.read_int_le() == -100000

    def test_int_be(self):
        w = PacketWriter()
        w.write_int_be(924)
        data = w.to_bytes()
        assert struct.unpack(">i", data)[0] == 924
        r = PacketReader(data)
        assert r.read_int_be() == 924

    def test_long_le(self):
        w = PacketWriter()
        w.write_long_le(1234567890123)
        r = PacketReader(w.to_bytes())
        assert r.read_long_le() == 1234567890123

    def test_float_le(self):
        w = PacketWriter()
        w.write_float_le(3.14)
        r = PacketReader(w.to_bytes())
        assert abs(r.read_float_le() - 3.14) < 0.001

    def test_bytes(self):
        w = PacketWriter()
        w.write_bytes(b"\x01\x02\x03")
        r = PacketReader(w.to_bytes())
        assert r.read_bytes(3) == b"\x01\x02\x03"


class TestReaderState:
    def test_position(self):
        r = PacketReader(b"\x00\x01\x02\x03")
        assert r.position == 0
        r.read_byte()
        assert r.position == 1

    def test_has_remaining(self):
        r = PacketReader(b"\x00")
        assert r.has_remaining()
        r.read_byte()
        assert not r.has_remaining()

    def test_remaining(self):
        r = PacketReader(b"\x00\x01\x02")
        assert r.remaining() == 3
        r.read_byte()
        assert r.remaining() == 2

    def test_skip(self):
        r = PacketReader(b"\x00\x01\x02\x03")
        r.skip(2)
        assert r.read_byte() == 0x02

    def test_read_remaining(self):
        r = PacketReader(b"\x00\x01\x02\x03")
        r.read_byte()
        assert r.read_remaining() == b"\x01\x02\x03"
        assert not r.has_remaining()
