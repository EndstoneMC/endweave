"""Tests for gameplay packet handlers (block position encoding changes).

Key: NetworkBlockPosition (v924) stores coords * 8 on wire, Y as unsigned varint.
     BlockPos (v944) stores raw coords on wire, Y as signed varint.
"""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.v924_to_v944.block_position import (
    NETWORK_BLOCK_POS_SCALE,
    read_block_pos,
    read_network_block_pos,
    write_block_pos,
    write_network_block_pos,
)
from endstone_endweave.protocol.v924_to_v944.handlers.command_block import rewrite_command_block_update
from endstone_endweave.protocol.v924_to_v944.handlers.map_item_data import rewrite_map_item_data
from endstone_endweave.protocol.v924_to_v944.handlers.start_game import rewrite_start_game
from endstone_endweave.protocol.v924_to_v944.handlers.structure_block import rewrite_structure_block_update
from endstone_endweave.protocol.v924_to_v944.handlers.sub_chunk import rewrite_sub_chunk_blocks


def _make_session() -> PlayerSession:
    return PlayerSession(address="1.2.3.4:1234", client_protocol=944, server_protocol=924)


S = NETWORK_BLOCK_POS_SCALE  # 8


class TestBlockPositionConversion:
    """Test the core block position conversion utilities."""

    def test_944_to_924_scales_up(self):
        """BlockPos -> NetworkBlockPosition should multiply by 8."""
        w = PacketWriter()
        write_block_pos(w, 10, 64, -30)
        r = PacketReader(w.to_bytes())

        # Read back as-is to verify the raw values
        x, y, z = read_block_pos(r)
        assert (x, y, z) == (10, 64, -30)

    def test_924_to_944_scales_down(self):
        """NetworkBlockPosition -> BlockPos should divide by 8."""
        w = PacketWriter()
        write_network_block_pos(w, 10 * S, 64 * S, -30 * S)
        r = PacketReader(w.to_bytes())

        x, y, z = read_network_block_pos(r)
        assert (x, y, z) == (80, 512, -240)  # raw wire values (* 8)

    def test_y_encoding_difference(self):
        """Y=64: unsigned varint encodes as 0x40, signed varint (zigzag) encodes as 0x80 0x01."""
        # Unsigned varint for 64 = single byte 0x40
        w1 = PacketWriter()
        w1.write_varint(64)
        assert w1.to_bytes() == bytes([0x40])

        # Signed varint (zigzag) for 64 = zigzag(64) = 128 = 0x80 0x01
        w2 = PacketWriter()
        w2.write_signed_varint(64)
        assert w2.to_bytes() == bytes([0x80, 0x01])


class TestCommandBlockUpdateRewriter:
    """Tests CommandBlockUpdatePacket (78) serverbound rewrite."""

    def setup_method(self):
        self.session = _make_session()

    def test_is_block_true_converts_pos(self):
        """When is_block=True, block position should be converted from BlockPos to NetworkBlockPosition."""
        w = PacketWriter()
        w.write_bool(True)  # is_block
        write_block_pos(w, 100, 64, -200)  # v944 BlockPos (raw coords)
        w.write_varint(1)   # command_block_mode
        w.write_bool(True)  # redstone_mode
        w.write_bool(False) # is_conditional
        w.write_string("say hello")  # command
        w.write_string("")  # last_output
        w.write_string("test")  # name
        w.write_string("")  # filtered_name
        w.write_bool(True)  # track_output
        w.write_uint_le(0)  # tick_delay
        w.write_bool(False) # execute_on_first_tick

        result = rewrite_command_block_update(w.to_bytes(), self.session)
        assert result.new_payload is not None

        # Verify output is v924 NetworkBlockPosition format (coords * 8)
        r = PacketReader(result.new_payload)
        assert r.read_bool() is True  # is_block
        x, y, z = read_network_block_pos(r)
        assert (x, y, z) == (100 * S, 64 * S, -200 * S)
        assert r.read_varint() == 1  # command_block_mode
        assert r.read_bool() is True  # redstone_mode
        assert r.read_bool() is False  # is_conditional
        assert r.read_string() == "say hello"

    def test_is_block_false_passes_through(self):
        """When is_block=False, runtime ID path has no block pos."""
        w = PacketWriter()
        w.write_bool(False)  # is_block
        w.write_varlong(42)  # target_runtime_id
        w.write_string("say hello")  # command

        result = rewrite_command_block_update(w.to_bytes(), self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        assert r.read_bool() is False
        assert r.read_varlong() == 42
        assert r.read_string() == "say hello"

    def test_y_coordinate_scaling(self):
        """Y=200 as BlockPos -> Y=1600 as NetworkBlockPosition unsigned varint."""
        w = PacketWriter()
        w.write_bool(True)
        write_block_pos(w, 0, 200, 0)
        w.write_varint(0)
        w.write_bool(False)
        w.write_bool(False)
        w.write_bytes(b"\x00" * 20)

        result = rewrite_command_block_update(w.to_bytes(), self.session)
        r = PacketReader(result.new_payload)
        assert r.read_bool() is True
        x, y, z = read_network_block_pos(r)
        assert y == 200 * S  # 1600


class TestStructureBlockUpdateRewriter:
    """Tests StructureBlockUpdatePacket (90) serverbound rewrite."""

    def setup_method(self):
        self.session = _make_session()

    def test_rewrites_block_pos_at_start(self):
        """Block position at start should be scaled and re-encoded."""
        w = PacketWriter()
        write_block_pos(w, 10, 200, -30)
        w.write_bytes(b"\xAA\xBB\xCC")  # remaining payload

        result = rewrite_structure_block_update(w.to_bytes(), self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        x, y, z = read_network_block_pos(r)
        assert (x, y, z) == (10 * S, 200 * S, -30 * S)
        assert r.read_remaining() == b"\xAA\xBB\xCC"


class TestSubChunkBlocksRewriter:
    """Tests UpdateSubChunkBlocksPacket (172) clientbound rewrite."""

    def setup_method(self):
        self.session = _make_session()

    def _build_v924_payload(self, blocks: list[tuple[int, int, int]]) -> bytes:
        """Build a v924 UpdateSubChunkBlocksPacket payload."""
        w = PacketWriter()
        # SubChunkPos (3x signed varint)
        w.write_signed_varint(0)
        w.write_signed_varint(4)
        w.write_signed_varint(0)

        # Standards array
        w.write_varint(len(blocks))
        for x, y, z in blocks:
            # NetworkBlockPosition: coords * 8, Y as unsigned varint
            write_network_block_pos(w, x * S, y * S, z * S)
            w.write_varint(1)   # runtime_id
            w.write_varint(0)   # update_flags
            w.write_varlong(0)  # entity_id
            w.write_varint(0)   # message_id

        # Extras array (empty)
        w.write_varint(0)

        return w.to_bytes()

    def test_rewrites_block_pos_with_scaling(self):
        """Block positions should be descaled from *8 to raw and re-encoded as signed."""
        payload = self._build_v924_payload([(5, 64, -3)])

        result = rewrite_sub_chunk_blocks(payload, self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        # SubChunkPos
        assert r.read_signed_varint() == 0
        assert r.read_signed_varint() == 4
        assert r.read_signed_varint() == 0

        # Standards
        count = r.read_varint()
        assert count == 1
        x, y, z = read_block_pos(r)
        assert (x, y, z) == (5, 64, -3)  # descaled from (40, 512, -24)

    def test_multiple_blocks(self):
        """Multiple blocks in arrays should all be converted."""
        w = PacketWriter()
        w.write_signed_varint(1)
        w.write_signed_varint(2)
        w.write_signed_varint(3)

        # Standards: 2 entries
        w.write_varint(2)
        for x, y, z in [(0, 10, 0), (1, 200, 1)]:
            write_network_block_pos(w, x * S, y * S, z * S)
            w.write_varint(1)
            w.write_varint(0)
            w.write_varlong(0)
            w.write_varint(0)

        # Extras: 1 entry
        w.write_varint(1)
        write_network_block_pos(w, 5 * S, 100 * S, 5 * S)
        w.write_varint(2)
        w.write_varint(0)
        w.write_varlong(0)
        w.write_varint(0)

        result = rewrite_sub_chunk_blocks(w.to_bytes(), self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        assert r.read_signed_varint() == 1
        assert r.read_signed_varint() == 2
        assert r.read_signed_varint() == 3

        # Standards
        assert r.read_varint() == 2
        for expected_y in [10, 200]:
            x, y, z = read_block_pos(r)
            assert y == expected_y
            r.read_varint(); r.read_varint(); r.read_varlong(); r.read_varint()

        # Extras
        assert r.read_varint() == 1
        x, y, z = read_block_pos(r)
        assert (x, y, z) == (5, 100, 5)

    def test_empty_arrays(self):
        """Empty block arrays should be handled gracefully."""
        w = PacketWriter()
        w.write_signed_varint(0)
        w.write_signed_varint(0)
        w.write_signed_varint(0)
        w.write_varint(0)  # standards: empty
        w.write_varint(0)  # extras: empty

        result = rewrite_sub_chunk_blocks(w.to_bytes(), self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        r.read_signed_varint(); r.read_signed_varint(); r.read_signed_varint()
        assert r.read_varint() == 0
        assert r.read_varint() == 0
        assert not r.has_remaining()


class TestStartGameRewriter:
    """Tests StartGamePacket (11) clientbound rewrite."""

    def setup_method(self):
        self.session = _make_session()

    def test_converts_spawn_position(self):
        """The spawn block position in LevelSettings should be descaled."""
        w = PacketWriter()
        # StartGamePacket fields before LevelSettings
        w.write_varlong(1)   # Entity ID (ActorUniqueID)
        w.write_varlong(2)   # Runtime ID (ActorRuntimeID)
        w.write_signed_varint(0)  # Game Type
        w.write_float_le(0.0); w.write_float_le(64.0); w.write_float_le(0.0)  # Position Vec3
        w.write_float_le(0.0); w.write_float_le(0.0)  # Rotation Vec2

        # LevelSettings
        w.write_long_le(12345)  # Seed (uint64)

        # SpawnSettings
        w.write_signed_varint(1)  # Generator Type
        w.write_signed_varint(0)  # Game Type
        w.write_bool(False)       # Hardcore
        w.write_signed_varint(2)  # Difficulty

        # Default Spawn Block Position as NetworkBlockPosition (v924 wire: coords * 8)
        write_network_block_pos(w, 100 * S, 64 * S, -50 * S)

        # Trailing data (rest of LevelSettings + rest of StartGamePacket)
        trailing = b"\xDE\xAD\xBE\xEF"
        w.write_bytes(trailing)

        result = rewrite_start_game(w.to_bytes(), self.session)
        assert result.new_payload is not None

        # Verify: read back and check the block pos is now BlockPos (raw coords)
        r = PacketReader(result.new_payload)
        r.read_varlong()  # Entity ID
        r.read_varlong()  # Runtime ID
        r.read_signed_varint()  # Game Type
        r.read_bytes(12)  # Position
        r.read_bytes(8)   # Rotation
        r.read_bytes(8)   # Seed
        r.read_signed_varint()  # Generator Type
        r.read_signed_varint()  # Game Type
        r.read_bool()     # Hardcore
        r.read_signed_varint()  # Difficulty

        # The spawn position should now be raw coords (descaled)
        x, y, z = read_block_pos(r)
        assert (x, y, z) == (100, 64, -50)

        assert r.read_remaining() == trailing


class TestMapItemDataRewriter:
    """Tests ClientboundMapItemDataPacket (67) clientbound rewrite."""

    def setup_method(self):
        self.session = _make_session()

    def _build_base_header(self, w: PacketWriter, type_flags: int) -> None:
        """Write the common header fields for a MapItemDataPacket."""
        w.write_varlong(42)       # Map ID
        w.write_varint(type_flags)  # Type Flags
        w.write_byte(0)           # Dimension
        w.write_bool(False)       # Is Locked Map?
        # Map Origin (BlockPos — already BlockPos in v924)
        w.write_signed_varint(100)
        w.write_signed_varint(64)
        w.write_signed_varint(-50)

    def test_no_decoration_passthrough(self):
        """Packet with only texture bit set should pass through without conversion."""
        w = PacketWriter()
        self._build_base_header(w, 0x08)  # Texture only
        w.write_byte(4)  # Scale
        # Texture data: width, height, x, y, pixels
        w.write_varint(1)   # width
        w.write_varint(1)   # height
        w.write_varint(0)   # x offset
        w.write_varint(0)   # y offset
        w.write_varint(1)   # pixel count
        w.write_uint_le(0xFF0000FF)  # one pixel (RGBA)

        payload = w.to_bytes()
        result = rewrite_map_item_data(payload, self.session)
        assert result.new_payload is not None

        # Verify header is preserved
        r = PacketReader(result.new_payload)
        assert r.read_varlong() == 42  # Map ID
        assert r.read_varint() == 0x08  # Type Flags
        assert r.read_byte() == 0  # Dimension

    def test_decoration_entity_type_no_conversion(self):
        """Decoration update with entity-type tracked actor should not convert positions."""
        w = PacketWriter()
        self._build_base_header(w, 0x02)  # Decoration bit
        w.write_byte(4)  # Scale
        # Actor IDs list
        w.write_varint(1)  # 1 tracked actor
        w.write_int_le(0)  # Type = Entity
        w.write_varlong(99)  # ActorUniqueID
        # Decoration list (opaque trailing data)
        trailing = b"\x00\xAA\xBB"
        w.write_bytes(trailing)

        result = rewrite_map_item_data(w.to_bytes(), self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        r.read_varlong()  # Map ID
        r.read_varint()   # Type Flags
        r.read_byte()     # Dimension
        r.read_bool()     # Is Locked
        for _ in range(3):
            r.read_signed_varint()  # Map Origin
        r.read_byte()     # Scale
        assert r.read_varint() == 1  # Actor ID count
        assert r.read_int_le() == 0  # Type = Entity
        assert r.read_varlong() == 99  # ActorUniqueID unchanged
        assert r.read_remaining() == trailing

    def test_decoration_block_entity_converts_pos(self):
        """Decoration update with block-entity tracked actor should convert NetworkBlockPosition."""
        w = PacketWriter()
        self._build_base_header(w, 0x02)  # Decoration bit
        w.write_byte(4)  # Scale
        # Actor IDs list
        w.write_varint(1)  # 1 tracked actor
        w.write_int_le(1)  # Type = BlockEntity
        # NetworkBlockPosition (v924): coords * 8
        write_network_block_pos(w, 10 * S, 64 * S, -30 * S)
        # Decoration list (opaque trailing data)
        trailing = b"\x00\xDD\xEE"
        w.write_bytes(trailing)

        result = rewrite_map_item_data(w.to_bytes(), self.session)
        assert result.new_payload is not None

        r = PacketReader(result.new_payload)
        r.read_varlong()  # Map ID
        r.read_varint()   # Type Flags
        r.read_byte()     # Dimension
        r.read_bool()     # Is Locked
        for _ in range(3):
            r.read_signed_varint()  # Map Origin
        r.read_byte()     # Scale
        assert r.read_varint() == 1  # Actor ID count
        assert r.read_int_le() == 1  # Type = BlockEntity
        # Should now be BlockPos (raw coords, descaled from *8)
        x, y, z = read_block_pos(r)
        assert (x, y, z) == (10, 64, -30)
        assert r.read_remaining() == trailing
