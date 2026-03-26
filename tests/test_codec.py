"""Tests for the binary codec, type singletons, and compound type roundtrips."""

import struct

import pytest
from helpers import (
    nbt_string,
    nbt_varint,
    nbt_varint64,
    read_block_pos,
    read_net_block_pos,
    write_block_pos,
    write_net_block_pos,
)

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    EXPERIMENTS,
    FLOAT_LE,
    GAME_RULES,
    INT64_LE,
    INT_BE,
    INT_LE,
    NAMED_COMPOUND_TAG,
    NETWORK_BLOCK_POS,
    REMAINING_BYTES,
    SHORT_LE,
    STRING,
    UINT_LE,
    USHORT_LE,
    UUID,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    ByteTag,
    CompoundTag,
    PacketReader,
    PacketWrapper,
)
from endstone_endweave.codec.types import ITEM_INSTANCE, ItemInstance
from endstone_endweave.codec.types.nbt import read_nbt
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
        w.write_int64_le(1234567890123)
        r = PacketReader(w.to_bytes())
        assert r.read_int64_le() == 1234567890123

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
        assert not r.has_remaining

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
        assert r.has_remaining
        r.read_byte()
        assert not r.has_remaining

    def test_remaining(self):
        r = PacketReader(b"\x00\x01\x02")
        assert r.remaining == 3
        r.read_byte()
        assert r.remaining == 2

    def test_skip(self):
        r = PacketReader(b"\x00\x01\x02\x03")
        r.skip(2)
        assert r.read_byte() == 0x02

    def test_read_remaining(self):
        r = PacketReader(b"\x00\x01\x02\x03")
        r.read_byte()
        assert r.read_remaining() == b"\x01\x02\x03"
        assert not r.has_remaining


class TestNetworkItemInstanceDescriptor:
    def test_air_roundtrip(self):
        item = ItemInstance(network_id=0)
        w = PacketWriter()
        ITEM_INSTANCE.write(w, item)
        r = PacketReader(w.to_bytes())
        result = ITEM_INSTANCE.read(r)
        assert result.network_id == 0
        assert not r.has_remaining

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
        assert not r.has_remaining

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
        assert not r.has_remaining

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


# ---------------------------------------------------------------------------
# Tests: Type singleton roundtrips via PacketWrapper
# ---------------------------------------------------------------------------


class TestTypeRoundtrips:
    """Verify each type reads what it writes through the wrapper."""

    def _roundtrip(self, field_type, value):
        w = PacketWriter()
        field_type.write(w, value)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        result = wrapper.passthrough(field_type)
        assert result == value
        assert wrapper.to_bytes() == payload

    def test_byte(self):
        self._roundtrip(BYTE, 0xFF)

    def test_bool_true(self):
        self._roundtrip(BOOL, True)

    def test_bool_false(self):
        self._roundtrip(BOOL, False)

    def test_short_le(self):
        self._roundtrip(SHORT_LE, -1234)

    def test_ushort_le(self):
        self._roundtrip(USHORT_LE, 65535)

    def test_int_le(self):
        self._roundtrip(INT_LE, -100000)

    def test_int_be(self):
        self._roundtrip(INT_BE, 924)

    def test_uint_le(self):
        self._roundtrip(UINT_LE, 0xDEADBEEF)

    def test_long_le(self):
        self._roundtrip(INT64_LE, 1234567890123)

    def test_float_le(self):
        w = PacketWriter()
        FLOAT_LE.write(w, 3.14)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        result = wrapper.passthrough(FLOAT_LE)
        assert abs(result - 3.14) < 0.001

    def test_var_int(self):
        for val in [0, 1, -1, 42, -42, 2147483647, -2147483648]:
            self._roundtrip(VAR_INT, val)

    def test_uvar_int(self):
        for val in [0, 1, 127, 128, 300, 0xFFFFFFFF]:
            self._roundtrip(UVAR_INT, val)

    def test_var_long(self):
        for val in [0, 1, -1, 2147483647]:
            self._roundtrip(VAR_INT64, val)

    def test_uvar_long(self):
        for val in [0, 1, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF]:
            self._roundtrip(UVAR_INT64, val)

    def test_string(self):
        self._roundtrip(STRING, "hello world")

    def test_string_empty(self):
        self._roundtrip(STRING, "")

    def test_uuid(self):
        self._roundtrip(UUID, b"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10")

    def test_remaining_bytes(self):
        self._roundtrip(REMAINING_BYTES, b"\xde\xad\xbe\xef")


# ---------------------------------------------------------------------------
# Tests: BlockPos / NetworkBlockPos type singletons
# ---------------------------------------------------------------------------


class TestBlockPosTypes:
    def test_network_block_pos_roundtrip(self):
        w = PacketWriter()
        NETWORK_BLOCK_POS.write(w, (10, 64, -20))
        r = PacketReader(w.to_bytes())
        assert NETWORK_BLOCK_POS.read(r) == (10, 64, -20)

    def test_block_pos_roundtrip(self):
        w = PacketWriter()
        BLOCK_POS.write(w, (10, 64, -20))
        r = PacketReader(w.to_bytes())
        assert BLOCK_POS.read(r) == (10, 64, -20)

    def test_encoding_difference(self):
        """uvarint(64) != varint(64) -- this is the core difference."""
        w_u = PacketWriter()
        w_u.write_uvarint(64)
        w_v = PacketWriter()
        w_v.write_varint(64)
        assert w_u.to_bytes() != w_v.to_bytes()

    def test_passthrough_network_block_pos(self):
        w = PacketWriter()
        write_net_block_pos(w, 5, 100, -5)
        w.write_byte(0xFF)
        wrapper = PacketWrapper(w.to_bytes())
        pos = wrapper.passthrough(NETWORK_BLOCK_POS)
        assert pos == (5, 100, -5)
        assert wrapper.to_bytes() == w.to_bytes()

    def test_network_block_pos_negative_y(self):
        """Negative Y must survive roundtrip (uvarint reinterpreted as signed)."""
        for y in (-1, -10, -64):
            w = PacketWriter()
            NETWORK_BLOCK_POS.write(w, (100, y, 50))
            r = PacketReader(w.to_bytes())
            assert NETWORK_BLOCK_POS.read(r) == (100, y, 50)

    def test_passthrough_network_block_pos_negative_y(self):
        """Passthrough with negative Y must produce identical bytes."""
        w = PacketWriter()
        write_net_block_pos(w, 5, -10, -5)
        wrapper = PacketWrapper(w.to_bytes())
        pos = wrapper.passthrough(NETWORK_BLOCK_POS)
        assert pos == (5, -10, -5)
        assert wrapper.to_bytes() == w.to_bytes()


# ---------------------------------------------------------------------------
# Tests: Cross-type conversion (NetworkBlockPos <-> BlockPos)
# ---------------------------------------------------------------------------


class TestTransformReadWrite:
    def test_net_block_to_block(self):
        w = PacketWriter()
        write_net_block_pos(w, 10, 64, -30)
        w.write_byte(0xAA)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert read_block_pos(r) == (10, 64, -30)
        assert r.read_byte() == 0xAA

    def test_block_to_net_block(self):
        w = PacketWriter()
        write_block_pos(w, -5, 200, 42)
        w.write_byte(0xBB)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert read_net_block_pos(r) == (-5, 200, 42)
        assert r.read_byte() == 0xBB

    def test_y_zero(self):
        """Y=0: uvarint(0) == varint(0) == 0x00, but verify correctness."""
        w = PacketWriter()
        write_net_block_pos(w, 0, 0, 0)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
        r = PacketReader(wrapper.to_bytes())
        assert read_block_pos(r) == (0, 0, 0)

    def test_large_y(self):
        """Y=320 (max overworld build height)."""
        w = PacketWriter()
        write_net_block_pos(w, 100, 320, -100)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
        r = PacketReader(wrapper.to_bytes())
        assert read_block_pos(r) == (100, 320, -100)

    def test_net_block_to_block_negative_y(self):
        """NetworkBlockPos -> BlockPos with negative Y (issue #3)."""
        for y in (-1, -10, -64):
            w = PacketWriter()
            write_net_block_pos(w, 100, y, 50)
            wrapper = PacketWrapper(w.to_bytes())
            wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
            r = PacketReader(wrapper.to_bytes())
            assert read_block_pos(r) == (100, y, 50)

    def test_block_to_net_block_negative_y(self):
        """BlockPos -> NetworkBlockPos with negative Y."""
        for y in (-1, -10, -64):
            w = PacketWriter()
            write_block_pos(w, 100, y, 50)
            wrapper = PacketWrapper(w.to_bytes())
            wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))
            r = PacketReader(wrapper.to_bytes())
            assert read_net_block_pos(r) == (100, y & 0xFFFFFFFF, 50)


# ---------------------------------------------------------------------------
# Tests: NBT compound parsing
# ---------------------------------------------------------------------------


class TestSkipNbtCompound:
    def test_empty_compound(self):
        """Root compound with no children (just End byte)."""
        data = bytes([10]) + nbt_string("") + bytes([0])
        r = PacketReader(data + b"\xff")
        read_nbt(r)
        assert r.read_byte() == 0xFF

    def test_null_root(self):
        """Root type byte is 0 (null/end) -- returns None (absent NBT)."""
        r = PacketReader(bytes([0]) + b"\xab")
        assert read_nbt(r) is None
        assert r.read_byte() == 0xAB

    def test_compound_with_primitives(self):
        """Compound containing Byte, Short, Int, Int64, Float, Double."""
        buf = bytearray()
        buf.append(10)
        buf.extend(nbt_string("root"))
        buf.append(1)
        buf.extend(nbt_string("b"))
        buf.append(42)
        buf.append(2)
        buf.extend(nbt_string("s"))
        buf.extend(struct.pack("<h", -100))
        buf.append(3)
        buf.extend(nbt_string("i"))
        buf.extend(nbt_varint(999))
        buf.append(4)
        buf.extend(nbt_string("l"))
        buf.extend(nbt_varint64(123456789))
        buf.append(5)
        buf.extend(nbt_string("f"))
        buf.extend(struct.pack("<f", 3.14))
        buf.append(6)
        buf.extend(nbt_string("d"))
        buf.extend(struct.pack("<d", 2.718))
        buf.append(0)
        buf.extend(b"\xee")
        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xEE

    def test_compound_with_string_and_arrays(self):
        """Compound with String, ByteArray, IntArray."""
        buf = bytearray()
        buf.append(10)
        buf.extend(nbt_string(""))
        buf.append(8)
        buf.extend(nbt_string("str"))
        buf.extend(nbt_string("hello"))
        buf.append(7)
        buf.extend(nbt_string("ba"))
        buf.extend(nbt_varint(3))
        buf.extend(b"\x01\x02\x03")
        buf.append(11)
        buf.extend(nbt_string("ia"))
        buf.extend(nbt_varint(2))
        buf.extend(nbt_varint(10))
        buf.extend(nbt_varint(20))
        buf.append(0)
        buf.extend(b"\xdd")
        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xDD

    def test_nested_compound(self):
        """Compound containing a nested compound."""
        buf = bytearray()
        buf.append(10)
        buf.extend(nbt_string(""))
        buf.append(10)
        buf.extend(nbt_string("inner"))
        buf.append(1)
        buf.extend(nbt_string("x"))
        buf.append(7)
        buf.append(0)
        buf.append(0)
        buf.extend(b"\xcc")
        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xCC

    def test_list_of_ints(self):
        """Compound containing a List of Ints."""
        buf = bytearray()
        buf.append(10)
        buf.extend(nbt_string(""))
        buf.append(9)
        buf.extend(nbt_string("nums"))
        buf.append(3)
        buf.extend(nbt_varint(3))
        buf.extend(nbt_varint(100))
        buf.extend(nbt_varint(200))
        buf.extend(nbt_varint(300))
        buf.append(0)
        buf.extend(b"\xbb")
        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xBB

    def test_list_of_compounds(self):
        """Compound containing a List of Compounds."""
        buf = bytearray()
        buf.append(10)
        buf.extend(nbt_string(""))
        buf.append(9)
        buf.extend(nbt_string("items"))
        buf.append(10)
        buf.extend(nbt_varint(2))
        buf.append(1)
        buf.extend(nbt_string("a"))
        buf.append(1)
        buf.append(0)
        buf.append(3)
        buf.extend(nbt_string("b"))
        buf.extend(nbt_varint(42))
        buf.append(0)
        buf.append(0)
        buf.extend(b"\xaa")
        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xAA

    def test_compound_tag_type_passthrough(self):
        """COMPOUND_TAG type parses into CompoundTag and round-trips correctly."""
        buf = bytearray()
        buf.append(10)
        buf.extend(nbt_string(""))
        buf.append(1)
        buf.extend(nbt_string("x"))
        buf.append(5)
        buf.append(0)
        nbt_bytes = bytes(buf)
        wrapper = PacketWrapper(nbt_bytes + b"\x99")
        tag = wrapper.passthrough(NAMED_COMPOUND_TAG)
        assert isinstance(tag, CompoundTag)
        assert isinstance(tag["x"], ByteTag)
        assert tag["x"].value == 5
        trailing = wrapper.passthrough_all()
        assert trailing == b"\x99"
        assert wrapper.to_bytes() == nbt_bytes + b"\x99"


# ---------------------------------------------------------------------------
# Tests: GameRules type passthrough
# ---------------------------------------------------------------------------


class TestPassthroughGameRules:
    def test_zero_rules(self):
        w = PacketWriter()
        w.write_uvarint(0)
        w.write_byte(0xFF)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.passthrough(GAME_RULES)
        result = wrapper.to_bytes()
        assert result == w.to_bytes()

    def test_mixed_rule_types(self):
        w = PacketWriter()
        w.write_uvarint(3)
        w.write_string("rule_bool")
        w.write_bool(True)
        w.write_byte(1)
        w.write_bool(False)
        w.write_string("rule_int")
        w.write_bool(False)
        w.write_byte(2)
        w.write_varint(42)
        w.write_string("rule_float")
        w.write_bool(True)
        w.write_byte(3)
        w.write_float_le(1.5)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        wrapper.passthrough(GAME_RULES)
        assert wrapper.to_bytes() == payload


# ---------------------------------------------------------------------------
# Tests: Experiments type passthrough
# ---------------------------------------------------------------------------


class TestPassthroughExperiments:
    def test_zero_experiments(self):
        w = PacketWriter()
        w.write_uint_le(0)
        w.write_bool(False)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        wrapper.passthrough(EXPERIMENTS)
        wrapper.passthrough(BOOL)
        assert wrapper.to_bytes() == payload

    def test_with_experiments(self):
        w = PacketWriter()
        w.write_uint_le(2)
        w.write_string("exp1")
        w.write_bool(True)
        w.write_string("exp2")
        w.write_bool(False)
        w.write_bool(True)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        wrapper.passthrough(EXPERIMENTS)
        wrapper.passthrough(BOOL)
        assert wrapper.to_bytes() == payload
