"""Tests for the binary codec (PacketReader / PacketWriter)."""

import struct

import pytest

from endstone_endweave.codec import PacketReader
from endstone_endweave.codec.types import ITEM_INSTANCE, ItemInstance
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


class TestReaderBoundsChecks:
    def test_read_bytes_raises_on_overread(self):
        r = PacketReader(b"\x00\x01")
        with pytest.raises(ValueError, match="read_bytes"):
            r.read_bytes(3)

    def test_read_bytes_raises_on_negative(self):
        r = PacketReader(b"\x00\x01")
        with pytest.raises(ValueError, match="read_bytes"):
            r.read_bytes(-1)

    def test_read_bytes_exact_remaining_ok(self):
        r = PacketReader(b"\x00\x01\x02")
        assert r.read_bytes(3) == b"\x00\x01\x02"

    def test_skip_raises_past_end(self):
        r = PacketReader(b"\x00\x01")
        with pytest.raises(ValueError, match="skip"):
            r.skip(3)

    def test_skip_raises_negative(self):
        r = PacketReader(b"\x00\x01")
        with pytest.raises(ValueError, match="skip"):
            r.skip(-1)

    def test_skip_exact_remaining_ok(self):
        r = PacketReader(b"\x00\x01")
        r.skip(2)
        assert not r.has_remaining()

    def test_read_string_raises_on_oversized_length(self):
        """String with length prefix exceeding 131068 bytes."""
        w = PacketWriter()
        w.write_uvarint(200000)  # length prefix > 131068
        w.write_bytes(b"\x00" * 200000)
        r = PacketReader(w.to_bytes())
        with pytest.raises(ValueError, match="String length"):
            r.read_string()


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


class TestItemInstance:
    def test_air_roundtrip(self):
        item = ItemInstance(network_id=0)
        w = PacketWriter()
        ITEM_INSTANCE.write(w, item)
        r = PacketReader(w.to_bytes())
        result = ITEM_INSTANCE.read(r)
        assert result.network_id == 0
        assert not r.has_remaining()

    def test_full_roundtrip(self):
        item = ItemInstance(
            network_id=42,
            count=64,
            aux_value=7,
            has_net_id=True,
            stack_net_id=99,
            block_runtime_id=123,
            user_data=b"\xaa\xbb\xcc",
        )
        w = PacketWriter()
        ITEM_INSTANCE.write(w, item)
        r = PacketReader(w.to_bytes())
        result = ITEM_INSTANCE.read(r)
        assert result == item
        assert not r.has_remaining()

    def test_roundtrip_no_net_id(self):
        item = ItemInstance(
            network_id=10,
            count=1,
            aux_value=0,
            has_net_id=False,
            stack_net_id=0,
            block_runtime_id=5,
            user_data=b"",
        )
        w = PacketWriter()
        ITEM_INSTANCE.write(w, item)
        r = PacketReader(w.to_bytes())
        result = ITEM_INSTANCE.read(r)
        assert result == item
        assert not r.has_remaining()

    def test_byte_identical_passthrough(self):
        """Write -> bytes -> read -> write again produces identical bytes."""
        item = ItemInstance(
            network_id=42,
            count=64,
            aux_value=7,
            has_net_id=True,
            stack_net_id=99,
            block_runtime_id=123,
            user_data=b"\x01\x02\x03\x04",
        )
        w1 = PacketWriter()
        ITEM_INSTANCE.write(w1, item)
        original_bytes = w1.to_bytes()

        r = PacketReader(original_bytes)
        roundtripped = ITEM_INSTANCE.read(r)

        w2 = PacketWriter()
        ITEM_INSTANCE.write(w2, roundtripped)
        assert w2.to_bytes() == original_bytes
