"""Tests for StartGamePacket handler and supporting codec methods."""

import struct


from endstone_endweave.codec import PacketReader, PacketWrapper, NAMED_COMPOUND_TAG, CompoundTag, ByteTag
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.codec.types.nbt import read_nbt
from endstone_endweave.protocol.v924_to_v944.handlers.start_game import (
    rewrite_start_game,
    _passthrough_game_rules,
    _passthrough_experiments,
)


# ---------------------------------------------------------------------------
# Helper to build Bedrock network NBT bytes
# ---------------------------------------------------------------------------


def _nbt_string(s: str) -> bytes:
    """Encode a uvarint-prefixed UTF-8 string (Bedrock network NBT format)."""
    encoded = s.encode("utf-8")
    w = PacketWriter()
    w.write_uvarint(len(encoded))
    w.write_bytes(encoded)
    return w.to_bytes()


def _nbt_varint(val: int) -> bytes:
    w = PacketWriter()
    w.write_varint(val)
    return w.to_bytes()


def _nbt_varint64(val: int) -> bytes:
    w = PacketWriter()
    w.write_varint64(val)
    return w.to_bytes()


# ---------------------------------------------------------------------------
# Tests: skip_nbt_compound
# ---------------------------------------------------------------------------


class TestSkipNbtCompound:
    def test_empty_compound(self):
        """Root compound with no children (just End byte)."""
        data = bytes([10]) + _nbt_string("") + bytes([0])  # type=Compound, name="", End
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
        buf.append(10)  # root type = Compound
        buf.extend(_nbt_string("root"))  # root name
        # Byte tag
        buf.append(1)  # type = Byte
        buf.extend(_nbt_string("b"))  # name
        buf.append(42)  # value
        # Short tag
        buf.append(2)  # type = Short
        buf.extend(_nbt_string("s"))
        buf.extend(struct.pack("<h", -100))
        # Int tag (zigzag varint)
        buf.append(3)
        buf.extend(_nbt_string("i"))
        buf.extend(_nbt_varint(999))
        # Int64 tag (zigzag varint64)
        buf.append(4)
        buf.extend(_nbt_string("l"))
        buf.extend(_nbt_varint64(123456789))
        # Float tag
        buf.append(5)
        buf.extend(_nbt_string("f"))
        buf.extend(struct.pack("<f", 3.14))
        # Double tag
        buf.append(6)
        buf.extend(_nbt_string("d"))
        buf.extend(struct.pack("<d", 2.718))
        # End
        buf.append(0)
        buf.extend(b"\xee")  # sentinel

        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xEE

    def test_compound_with_string_and_arrays(self):
        """Compound with String, ByteArray, IntArray."""
        buf = bytearray()
        buf.append(10)
        buf.extend(_nbt_string(""))
        # String tag
        buf.append(8)
        buf.extend(_nbt_string("str"))
        buf.extend(_nbt_string("hello"))
        # ByteArray tag
        buf.append(7)
        buf.extend(_nbt_string("ba"))
        buf.extend(_nbt_varint(3))  # length=3
        buf.extend(b"\x01\x02\x03")
        # IntArray tag
        buf.append(11)
        buf.extend(_nbt_string("ia"))
        buf.extend(_nbt_varint(2))  # count=2
        buf.extend(_nbt_varint(10))
        buf.extend(_nbt_varint(20))
        # End
        buf.append(0)
        buf.extend(b"\xdd")

        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xDD

    def test_nested_compound(self):
        """Compound containing a nested compound."""
        buf = bytearray()
        buf.append(10)
        buf.extend(_nbt_string(""))
        # Nested compound
        buf.append(10)
        buf.extend(_nbt_string("inner"))
        buf.append(1)  # Byte in inner
        buf.extend(_nbt_string("x"))
        buf.append(7)
        buf.append(0)  # End inner
        buf.append(0)  # End root
        buf.extend(b"\xcc")

        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xCC

    def test_list_of_ints(self):
        """Compound containing a List of Ints."""
        buf = bytearray()
        buf.append(10)
        buf.extend(_nbt_string(""))
        # List tag
        buf.append(9)
        buf.extend(_nbt_string("nums"))
        buf.append(3)  # elem_type = Int
        buf.extend(_nbt_varint(3))  # count = 3
        buf.extend(_nbt_varint(100))
        buf.extend(_nbt_varint(200))
        buf.extend(_nbt_varint(300))
        buf.append(0)  # End
        buf.extend(b"\xbb")

        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xBB

    def test_list_of_compounds(self):
        """Compound containing a List of Compounds."""
        buf = bytearray()
        buf.append(10)
        buf.extend(_nbt_string(""))
        # List of compounds
        buf.append(9)
        buf.extend(_nbt_string("items"))
        buf.append(10)  # elem_type = Compound
        buf.extend(_nbt_varint(2))  # count = 2
        # First compound element
        buf.append(1)  # Byte tag
        buf.extend(_nbt_string("a"))
        buf.append(1)
        buf.append(0)  # End
        # Second compound element
        buf.append(3)  # Int tag
        buf.extend(_nbt_string("b"))
        buf.extend(_nbt_varint(42))
        buf.append(0)  # End
        # End root
        buf.append(0)
        buf.extend(b"\xaa")

        r = PacketReader(bytes(buf))
        read_nbt(r)
        assert r.read_byte() == 0xAA

    def test_compound_tag_type_passthrough(self):
        """COMPOUND_TAG type parses into CompoundTag and round-trips correctly."""
        buf = bytearray()
        buf.append(10)
        buf.extend(_nbt_string(""))
        buf.append(1)
        buf.extend(_nbt_string("x"))
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
# Tests: _passthrough_game_rules
# ---------------------------------------------------------------------------


class TestPassthroughGameRules:
    def test_zero_rules(self):
        w = PacketWriter()
        w.write_uvarint(0)
        w.write_byte(0xFF)
        wrapper = PacketWrapper(w.to_bytes())
        _passthrough_game_rules(wrapper)
        result = wrapper.to_bytes()
        # Output should contain the rules (0 count) + sentinel
        assert result == w.to_bytes()

    def test_mixed_rule_types(self):
        w = PacketWriter()
        w.write_uvarint(3)  # 3 rules
        # Rule 1: bool type
        w.write_string("rule_bool")
        w.write_bool(True)  # editable
        w.write_byte(1)  # type = bool
        w.write_bool(False)  # value
        # Rule 2: varint type
        w.write_string("rule_int")
        w.write_bool(False)
        w.write_byte(2)  # type = varint
        w.write_varint(42)
        # Rule 3: float type
        w.write_string("rule_float")
        w.write_bool(True)
        w.write_byte(3)  # type = float
        w.write_float_le(1.5)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        _passthrough_game_rules(wrapper)
        assert wrapper.to_bytes() == payload


# ---------------------------------------------------------------------------
# Tests: _passthrough_experiments
# ---------------------------------------------------------------------------


class TestPassthroughExperiments:
    def test_zero_experiments(self):
        w = PacketWriter()
        w.write_uint_le(0)  # count = 0
        w.write_bool(False)  # ever_toggled
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        _passthrough_experiments(wrapper)
        assert wrapper.to_bytes() == payload

    def test_with_experiments(self):
        w = PacketWriter()
        w.write_uint_le(2)  # count = 2
        w.write_string("exp1")
        w.write_bool(True)
        w.write_string("exp2")
        w.write_bool(False)
        w.write_bool(True)  # ever_toggled
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        _passthrough_experiments(wrapper)
        assert wrapper.to_bytes() == payload


# ---------------------------------------------------------------------------
# Tests: Full StartGamePacket rewrite
# ---------------------------------------------------------------------------


def _build_v924_start_game(
    has_server_join_info: bool, has_gathering: bool = False
) -> bytes:
    """Build a synthetic v924 StartGamePacket payload."""
    w = PacketWriter()

    # -- Fields before DefaultSpawn Y --
    w.write_varint64(1)  # mEntityId
    w.write_uvarint64(2)  # mRuntimeId
    w.write_varint(0)  # mEntityGameType (survival)
    w.write_float_le(1.0)  # mPos.X
    w.write_float_le(64.0)  # mPos.Y
    w.write_float_le(1.0)  # mPos.Z
    w.write_float_le(0.0)  # mRot.X
    w.write_float_le(0.0)  # mRot.Y
    w.write_long_le(12345)  # seed
    w.write_short_le(0)  # spawn settings type
    w.write_string("")  # biome name
    w.write_varint(0)  # dimension
    w.write_varint(1)  # generator
    w.write_varint(0)  # game type
    w.write_bool(False)  # is hardcore
    w.write_varint(1)  # difficulty

    # DefaultSpawn (v924 uses uvarint for Y)
    w.write_varint(0)  # X
    w.write_uvarint(64)  # Y (uvarint in v924)
    w.write_varint(0)  # Z

    # -- Remaining LevelSettings --
    w.write_bool(False)  # achievements disabled
    w.write_varint(0)  # editor world type
    w.write_bool(False)  # created in editor
    w.write_bool(False)  # exported from editor
    w.write_varint(0)  # day cycle stop time
    w.write_varint(0)  # education edition offer
    w.write_bool(False)  # education features
    w.write_string("")  # education product id
    w.write_float_le(0.0)  # rain level
    w.write_float_le(0.0)  # lightning level
    w.write_bool(False)  # platform locked content
    w.write_bool(True)  # multiplayer intended
    w.write_bool(True)  # LAN broadcasting
    w.write_varint(0)  # xbox broadcast
    w.write_varint(0)  # platform broadcast
    w.write_bool(True)  # commands enabled
    w.write_bool(False)  # texture packs required

    # GameRules: 1 bool rule
    w.write_uvarint(1)
    w.write_string("dodaylightcycle")
    w.write_bool(True)  # editable
    w.write_byte(1)  # type = bool
    w.write_bool(True)  # value

    # Experiments: 0
    w.write_uint_le(0)
    w.write_bool(False)  # ever_toggled

    w.write_bool(False)  # bonus chest
    w.write_bool(False)  # start with map
    w.write_varint(0)  # player permissions
    w.write_int_le(4)  # server chunk tick range
    w.write_bytes(b"\x00" * 10)  # 10 bools
    w.write_string("*")  # base game version
    w.write_int_le(0)  # limited world width
    w.write_int_le(0)  # limited world depth
    w.write_bool(False)  # nether type
    w.write_string("")  # edu uri button
    w.write_string("")  # edu uri link
    w.write_bool(False)  # force experimental
    w.write_byte(0)  # chat restriction
    w.write_bool(False)  # disable player interactions

    # -- Post-LevelSettings --
    w.write_string("level-id")  # level ID
    w.write_string("My World")  # level name
    w.write_string("")  # template content identity
    w.write_bool(False)  # is trial
    w.write_varint(0)  # rewind history size
    w.write_bool(True)  # server auth block breaking
    w.write_long_le(1000)  # level current time
    w.write_varint(0)  # enchantment seed

    # Block Properties: 0
    w.write_uvarint(0)

    w.write_string("correlation-id")  # multiplayer correlation id
    w.write_bool(True)  # item stack net manager

    w.write_string("1.26.0.50")  # server version

    # Player Property Data: empty compound
    w.write_byte(10)  # type = Compound
    w.write_uvarint(0)  # empty name
    w.write_byte(0)  # End tag

    w.write_long_le(0)  # block registry checksum
    w.write_long_le(0)  # world template MSB
    w.write_long_le(0)  # world template LSB
    w.write_bool(False)  # clientside generation
    w.write_bool(True)  # block network ids are hashes
    w.write_bool(False)  # server auth sound

    # -- Server join info (v924 format) --
    w.write_bool(has_server_join_info)
    if has_server_join_info:
        w.write_bool(has_gathering)
        if has_gathering:
            for s in [
                "exp-id",
                "exp-name",
                "exp-world-id",
                "exp-world-name",
                "creator-id",
                "store-id",
                "store-name",
            ]:
                w.write_string(s)

    # Trailing strings
    w.write_string("server-id")
    w.write_string("scenario-id")
    w.write_string("world-id")
    w.write_string("owner-id")

    return w.to_bytes()


class TestRewriteStartGame:
    def test_no_join_info(self):
        """v924 packet with has_server_join_info=false."""
        payload = _build_v924_start_game(has_server_join_info=False)
        wrapper = PacketWrapper(payload)
        rewrite_start_game(wrapper)
        result = wrapper.to_bytes()

        # Verify the output can be fully parsed -- read from the result
        # and check key fields
        r = PacketReader(result)
        assert r.read_varint64() == 1  # entity id
        assert r.read_uvarint64() == 2  # runtime id
        r.read_varint()  # game type
        for _ in range(4):
            r.read_float_le()  # pos + rot (partial)
        r.read_float_le()  # rot Y
        r.read_long_le()  # seed
        r.read_short_le()  # spawn settings type
        r.read_string()  # biome
        r.read_varint()  # dimension
        r.read_varint()  # generator
        r.read_varint()  # game type
        r.read_bool()  # hardcore
        r.read_varint()  # difficulty

        # DefaultSpawn -- Y should now be varint (v944 format)
        x = r.read_varint()
        y = r.read_varint()  # was uvarint, now varint
        z = r.read_varint()
        assert x == 0
        assert y == 64
        assert z == 0

        # We don't need to validate every field, but verify trailing strings
        # are at the end. Seek to near end by skipping the bulk.
        # Instead, verify the total output is valid by checking the wrapper
        # consumed all input.
        assert not wrapper.has_remaining()

    def test_with_join_info_stripped(self):
        """v924 gathering data is stripped; v944 sub-fields are written."""
        payload_with_gather = _build_v924_start_game(
            has_server_join_info=True, has_gathering=True
        )
        payload_no_gather = _build_v924_start_game(
            has_server_join_info=True, has_gathering=False
        )
        # The input with gathering is larger (7 extra strings)
        assert len(payload_with_gather) > len(payload_no_gather)

        wrapper_gather = PacketWrapper(payload_with_gather)
        rewrite_start_game(wrapper_gather)

        wrapper_no_gather = PacketWrapper(payload_no_gather)
        rewrite_start_game(wrapper_no_gather)

        # Both produce the same v944 output (gathering data stripped,
        # replaced with 3 False bools)
        assert wrapper_gather.to_bytes() == wrapper_no_gather.to_bytes()

    def test_join_info_true_no_gathering(self):
        """v924 packet with has_server_join_info=true but has_gathering=false."""
        payload = _build_v924_start_game(has_server_join_info=True, has_gathering=False)
        wrapper = PacketWrapper(payload)
        rewrite_start_game(wrapper)
        assert not wrapper.has_remaining()

    def test_trailing_strings_preserved(self):
        """Verify the 4 trailing strings survive the rewrite."""
        payload = _build_v924_start_game(has_server_join_info=False)
        wrapper = PacketWrapper(payload)
        rewrite_start_game(wrapper)
        result = wrapper.to_bytes()

        # Verify trailing strings are present in the output
        assert b"server-id" in result
        assert b"scenario-id" in result
        assert b"world-id" in result
        assert b"owner-id" in result

    def test_all_input_consumed(self):
        """Ensure the handler consumes the entire v924 packet."""
        for has_join in [False, True]:
            for has_gather in [False, True]:
                if not has_join and has_gather:
                    continue  # gathering only relevant when join info present
                payload = _build_v924_start_game(has_join, has_gather)
                wrapper = PacketWrapper(payload)
                rewrite_start_game(wrapper)
                assert not wrapper.has_remaining(), (
                    f"Unread bytes remain (join={has_join}, gather={has_gather})"
                )
