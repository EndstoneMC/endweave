"""Tests for the Bedrock LE NBT compound skipper."""

import struct

from endstone_endweave.codec import PacketReader
from endstone_endweave.codec.nbt import skip_nbt_compound


def _build_nbt(*parts: bytes) -> bytes:
    return b"".join(parts)


def _compound_start(name: str = "") -> bytes:
    encoded = name.encode("utf-8")
    return b"\x0a" + struct.pack("<H", len(encoded)) + encoded


def _tag(tag_type: int, name: str, payload: bytes) -> bytes:
    encoded = name.encode("utf-8")
    return bytes([tag_type]) + struct.pack("<H", len(encoded)) + encoded + payload


def _end() -> bytes:
    return b"\x00"


def _string_payload(val: str) -> bytes:
    encoded = val.encode("utf-8")
    return struct.pack("<H", len(encoded)) + encoded


class TestSkipNbtCompound:
    def test_empty_compound(self):
        data = _compound_start("") + _end() + b"\xff"
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0xFF

    def test_primitives(self):
        data = _build_nbt(
            _compound_start("root"),
            _tag(1, "byte_val", b"\x42"),
            _tag(2, "short_val", struct.pack("<h", 1234)),
            _tag(3, "int_val", struct.pack("<i", 99999)),
            _tag(4, "long_val", struct.pack("<q", 123456789)),
            _tag(5, "float_val", struct.pack("<f", 3.14)),
            _tag(6, "double_val", struct.pack("<d", 2.718)),
            _end(),
            b"\xab",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0xAB

    def test_nested_compounds(self):
        inner = (
            _tag(10, "inner", b"")
            + _tag(1, "x", b"\x01")
            + _end()
        )
        data = _build_nbt(
            _compound_start("outer"),
            inner,
            _end(),
            b"\xcd",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0xCD

    def test_string_tag(self):
        data = _build_nbt(
            _compound_start(""),
            _tag(8, "msg", _string_payload("hello world")),
            _end(),
            b"\xef",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0xEF

    def test_byte_array(self):
        arr = b"\x01\x02\x03\x04\x05"
        data = _build_nbt(
            _compound_start(""),
            _tag(7, "arr", struct.pack("<i", len(arr)) + arr),
            _end(),
            b"\xde",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0xDE

    def test_int_array(self):
        ints = struct.pack("<3i", 10, 20, 30)
        data = _build_nbt(
            _compound_start(""),
            _tag(11, "ints", struct.pack("<i", 3) + ints),
            _end(),
            b"\xba",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0xBA

    def test_long_array(self):
        longs = struct.pack("<2q", 100, 200)
        data = _build_nbt(
            _compound_start(""),
            _tag(12, "longs", struct.pack("<i", 2) + longs),
            _end(),
            b"\x99",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0x99

    def test_list_of_ints(self):
        # List tag: element type (byte) + count (int LE) + elements
        list_payload = b"\x03" + struct.pack("<i", 3) + struct.pack("<3i", 1, 2, 3)
        data = _build_nbt(
            _compound_start(""),
            _tag(9, "nums", list_payload),
            _end(),
            b"\x77",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0x77

    def test_list_of_compounds(self):
        # List of compounds: each element is a compound payload (no type byte or name)
        elem = _tag(1, "val", b"\x0a") + _end()
        list_payload = b"\x0a" + struct.pack("<i", 2) + elem + elem
        data = _build_nbt(
            _compound_start(""),
            _tag(9, "items", list_payload),
            _end(),
            b"\x55",
        )
        reader = PacketReader(data)
        skip_nbt_compound(reader)
        assert reader.read_byte() == 0x55
