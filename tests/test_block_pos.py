"""Tests for BlockPos types and packet handlers (v924 NetworkBlockPos <-> v944 BlockPos)."""

from endstone_endweave.codec import (
    BLOCK_POS,
    NETWORK_BLOCK_POS,
    PacketWrapper,
)
from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.protocol.v924_to_v944.handlers.block_pos import (
    rewrite_add_volume_entity,
    rewrite_anvil_damage,
    rewrite_command_block_update,
    rewrite_container_open,
    rewrite_first_net_block_to_block,
    rewrite_play_sound,
    rewrite_player_action,
    rewrite_set_spawn_position,
    rewrite_structure_block_update,
    rewrite_structure_template_data_request,
    rewrite_update_client_input_locks,
    rewrite_update_sub_chunk_blocks,
)
from endstone_endweave.protocol.v924_to_v944.handlers.camera import (
    rewrite_camera_spline,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _write_net_block_pos(w: PacketWriter, x: int, y: int, z: int) -> None:
    """Write a v924 NetworkBlockPos."""
    w.write_varint(x)
    w.write_uvarint(y)
    w.write_varint(z)


def _write_block_pos(w: PacketWriter, x: int, y: int, z: int) -> None:
    """Write a v944 BlockPos."""
    w.write_varint(x)
    w.write_varint(y)
    w.write_varint(z)


def _read_block_pos(r: PacketReader) -> tuple[int, int, int]:
    """Read a v944 BlockPos."""
    return (r.read_varint(), r.read_varint(), r.read_varint())


def _read_net_block_pos(r: PacketReader) -> tuple[int, int, int]:
    """Read a v924 NetworkBlockPos."""
    return (r.read_varint(), r.read_uvarint(), r.read_varint())


def _write_structure_settings_v944(w: PacketWriter) -> None:
    """Write a StructureSettings in v944 format (BlockPos for Size/Offset)."""
    w.write_string("palette")  # PaletteName
    w.write_bool(False)  # IgnoreEntities
    w.write_bool(False)  # IgnoreBlocks
    w.write_bool(True)  # AllowNonTickingChunks
    _write_block_pos(w, 10, 20, 30)  # Size (v944 BlockPos)
    _write_block_pos(w, -1, 5, -1)  # Offset (v944 BlockPos)
    w.write_varint64(123)  # LastEditingPlayerUniqueID
    w.write_byte(0)  # Rotation
    w.write_byte(0)  # Mirror
    w.write_byte(0)  # AnimationMode
    w.write_float_le(0.0)  # AnimationSeconds
    w.write_float_le(100.0)  # IntegrityValue
    w.write_uint_le(42)  # IntegritySeed
    w.write_float_le(0.5)  # RotationPivot.X
    w.write_float_le(0.5)  # RotationPivot.Y
    w.write_float_le(0.5)  # RotationPivot.Z


def _verify_structure_settings_v924(r: PacketReader) -> None:
    """Verify StructureSettings was converted to v924 (NetworkBlockPos for Size/Offset)."""
    assert r.read_string() == "palette"
    assert r.read_bool() is False  # IgnoreEntities
    assert r.read_bool() is False  # IgnoreBlocks
    assert r.read_bool() is True  # AllowNonTickingChunks
    assert _read_net_block_pos(r) == (10, 20, 30)  # Size
    assert _read_net_block_pos(r) == (-1, 5, -1)  # Offset
    assert r.read_varint64() == 123  # LastEditingPlayerUniqueID
    assert r.read_byte() == 0  # Rotation
    assert r.read_byte() == 0  # Mirror
    assert r.read_byte() == 0  # AnimationMode
    assert r.read_float_le() == 0.0  # AnimationSeconds
    assert r.read_float_le() == 100.0  # IntegrityValue
    assert r.read_uint_le() == 42  # IntegritySeed


# ---------------------------------------------------------------------------
# Tests: Type singletons
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
        _write_net_block_pos(w, 5, 100, -5)
        w.write_byte(0xFF)
        wrapper = PacketWrapper(w.to_bytes())
        pos = wrapper.passthrough(NETWORK_BLOCK_POS)
        assert pos == (5, 100, -5)
        assert wrapper.to_bytes() == w.to_bytes()


# ---------------------------------------------------------------------------
# Tests: Read/write transform (NetworkBlockPos -> BlockPos and back)
# ---------------------------------------------------------------------------


class TestTransformReadWrite:
    def test_net_block_to_block(self):
        w = PacketWriter()
        _write_net_block_pos(w, 10, 64, -30)
        w.write_byte(0xAA)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert _read_block_pos(r) == (10, 64, -30)
        assert r.read_byte() == 0xAA

    def test_block_to_net_block(self):
        w = PacketWriter()
        _write_block_pos(w, -5, 200, 42)
        w.write_byte(0xBB)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert _read_net_block_pos(r) == (-5, 200, 42)
        assert r.read_byte() == 0xBB

    def test_y_zero(self):
        """Y=0: uvarint(0) == varint(0) == 0x00, but verify correctness."""
        w = PacketWriter()
        _write_net_block_pos(w, 0, 0, 0)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (0, 0, 0)

    def test_large_y(self):
        """Y=320 (max overworld build height)."""
        w = PacketWriter()
        _write_net_block_pos(w, 100, 320, -100)
        wrapper = PacketWrapper(w.to_bytes())
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (100, 320, -100)


# ---------------------------------------------------------------------------
# Tests: Clientbound packet handlers
# ---------------------------------------------------------------------------


class TestClientboundHandlers:
    def test_first_field_update_block(self):
        w = PacketWriter()
        _write_net_block_pos(w, 10, 64, 20)
        w.write_uvarint(5)  # blockRuntimeID
        w.write_uvarint(3)  # flags
        w.write_uvarint(1)  # layer
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_first_net_block_to_block(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (10, 64, 20)
        assert r.read_uvarint() == 5
        assert r.read_uvarint() == 3
        assert r.read_uvarint() == 1

    def test_first_field_block_event(self):
        w = PacketWriter()
        _write_net_block_pos(w, -10, 80, 30)
        w.write_varint(1)  # eventType
        w.write_varint(2)  # eventData
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_first_net_block_to_block(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (-10, 80, 30)
        assert r.read_varint() == 1
        assert r.read_varint() == 2

    def test_set_spawn_position(self):
        w = PacketWriter()
        w.write_varint(1)  # spawnType
        _write_net_block_pos(w, 10, 64, 20)  # Position
        w.write_varint(0)  # dimension
        _write_net_block_pos(w, 30, 128, 40)  # SpawnPosition
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_set_spawn_position(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_varint() == 1
        assert _read_block_pos(r) == (10, 64, 20)
        assert r.read_varint() == 0
        assert _read_block_pos(r) == (30, 128, 40)

    def test_first_field_block_actor_data(self):
        w = PacketWriter()
        _write_net_block_pos(w, 5, 70, 15)
        w.write_bytes(b"\xde\xad")  # fake NBT tail
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_first_net_block_to_block(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (5, 70, 15)
        assert r.read_remaining() == b"\xde\xad"

    def test_add_volume_entity(self):
        w = PacketWriter()
        w.write_uvarint(42)  # RuntimeID
        _write_net_block_pos(w, 0, 0, 0)  # MinBound
        _write_net_block_pos(w, 16, 256, 16)  # MaxBound
        w.write_byte(0xFF)  # trailing
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_add_volume_entity(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == 42
        assert _read_block_pos(r) == (0, 0, 0)
        assert _read_block_pos(r) == (16, 256, 16)
        assert r.read_byte() == 0xFF

    def test_update_sub_chunk_blocks_empty(self):
        w = PacketWriter()
        _write_net_block_pos(w, 10, 64, 20)  # SubChunk position
        w.write_uvarint(0)  # blocks count = 0
        w.write_uvarint(0)  # extra count = 0
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_sub_chunk_blocks(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (10, 64, 20)
        assert r.read_uvarint() == 0
        assert r.read_uvarint() == 0
        assert not r.has_remaining

    def test_update_sub_chunk_blocks_with_entries(self):
        w = PacketWriter()
        _write_net_block_pos(w, 0, 4, 0)  # SubChunk position
        # 2 block entries
        w.write_uvarint(2)
        for pos_y in [64, 65]:
            _write_net_block_pos(w, 1, pos_y, 2)
            w.write_uvarint(100)  # blockRuntimeID
            w.write_uvarint(3)  # flags
            w.write_uvarint64(0)  # syncedUpdateEntityUniqueID
            w.write_uvarint(0)  # syncedUpdateType
        # 1 extra entry
        w.write_uvarint(1)
        _write_net_block_pos(w, 3, 66, 4)
        w.write_uvarint(200)
        w.write_uvarint(1)
        w.write_uvarint64(99)
        w.write_uvarint(1)  # syncedUpdateType
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_sub_chunk_blocks(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (0, 4, 0)
        assert r.read_uvarint() == 2
        assert _read_block_pos(r) == (1, 64, 2)
        assert r.read_uvarint() == 100
        assert r.read_uvarint() == 3
        assert r.read_uvarint64() == 0
        assert r.read_uvarint() == 0  # syncedUpdateType
        assert _read_block_pos(r) == (1, 65, 2)
        assert r.read_uvarint() == 100
        assert r.read_uvarint() == 3
        assert r.read_uvarint64() == 0
        assert r.read_uvarint() == 0  # syncedUpdateType
        assert r.read_uvarint() == 1
        assert _read_block_pos(r) == (3, 66, 4)
        assert r.read_uvarint() == 200
        assert r.read_uvarint() == 1
        assert r.read_uvarint64() == 99
        assert r.read_uvarint() == 1  # syncedUpdateType
        assert not r.has_remaining

    def test_first_field_open_sign(self):
        w = PacketWriter()
        _write_net_block_pos(w, 7, 72, -3)
        w.write_bool(True)  # isFrontSide
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_first_net_block_to_block(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_block_pos(r) == (7, 72, -3)
        assert r.read_bool() is True

    def test_update_client_input_locks(self):
        w = PacketWriter()
        w.write_uvarint(0x0F)  # Locks
        w.write_float_le(1.0)  # Position.X (to be stripped)
        w.write_float_le(64.0)  # Position.Y
        w.write_float_le(1.0)  # Position.Z
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_client_input_locks(wrapper)
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert r.read_uvarint() == 0x0F
        assert not r.has_remaining

    def test_camera_spline_zero(self):
        w = PacketWriter()
        w.write_uvarint(0)  # Camera Data Splines count
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_spline(wrapper)
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert r.read_uvarint() == 0
        assert not r.has_remaining

    def test_play_sound(self):
        w = PacketWriter()
        w.write_string("mob.zombie.say")  # Name
        _write_net_block_pos(w, 100, 64, -200)  # Position
        w.write_float_le(1.0)  # Volume
        w.write_float_le(0.8)  # Pitch
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_play_sound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_string() == "mob.zombie.say"
        assert _read_block_pos(r) == (100, 64, -200)
        assert r.read_float_le() == 1.0  # Volume
        assert r.remaining == 4  # Pitch passthrough

    def test_container_open(self):
        w = PacketWriter()
        w.write_byte(5)  # windowID
        w.write_byte(2)  # type
        _write_net_block_pos(w, -3, 80, 7)  # ContainerPosition
        w.write_varint64(-1)  # entityUniqueID
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_container_open(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_byte() == 5
        assert r.read_byte() == 2
        assert _read_block_pos(r) == (-3, 80, 7)
        assert r.read_varint64() == -1


# ---------------------------------------------------------------------------
# Tests: Serverbound packet handlers
# ---------------------------------------------------------------------------


class TestServerboundHandlers:
    def test_player_action(self):
        w = PacketWriter()
        w.write_uvarint64(1)  # entityRuntimeID
        w.write_varint(2)  # actionType
        _write_block_pos(w, 10, 64, 20)  # BlockPosition (v944 format)
        _write_block_pos(w, 11, 65, 21)  # ResultPosition
        w.write_varint(1)  # face
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_player_action(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 1
        assert r.read_varint() == 2
        assert _read_net_block_pos(r) == (10, 64, 20)
        assert _read_net_block_pos(r) == (11, 65, 21)
        assert r.read_varint() == 1

    def test_command_block_update_is_block(self):
        w = PacketWriter()
        w.write_bool(True)  # isBlock
        _write_block_pos(w, 100, 64, 200)  # Position
        w.write_bytes(b"\xaa\xbb")  # rest of packet
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_command_block_update(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is True
        assert _read_net_block_pos(r) == (100, 64, 200)
        assert r.read_remaining() == b"\xaa\xbb"

    def test_command_block_update_not_block(self):
        w = PacketWriter()
        w.write_bool(False)  # isBlock = false -> no BlockPos
        w.write_bytes(b"\xcc\xdd")
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_command_block_update(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is False
        assert r.read_remaining() == b"\xcc\xdd"

    def test_structure_block_update(self):
        w = PacketWriter()
        _write_block_pos(w, 5, 100, -5)  # Block Position
        # StructureEditorData
        w.write_string("my_struct")  # Name
        w.write_string("")  # DataField
        w.write_bool(False)  # IncludePlayers
        w.write_bool(True)  # ShowBoundingBox
        w.write_varint(1)  # StructureBlockType
        _write_structure_settings_v944(w)
        w.write_varint(0)  # RedstoneSaveMode
        # Remaining fields
        w.write_bool(True)  # Trigger
        w.write_bool(False)  # IsWaterlogged
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_structure_block_update(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert _read_net_block_pos(r) == (5, 100, -5)
        assert r.read_string() == "my_struct"
        assert r.read_string() == ""
        assert r.read_bool() is False
        assert r.read_bool() is True
        assert r.read_varint() == 1
        _verify_structure_settings_v924(r)

    def test_structure_template_data_request(self):
        w = PacketWriter()
        w.write_string("my_structure")  # Name
        _write_block_pos(w, 0, 64, 0)  # Position
        _write_structure_settings_v944(w)
        w.write_byte(1)  # RequestedOperation
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_structure_template_data_request(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_string() == "my_structure"
        assert _read_net_block_pos(r) == (0, 64, 0)
        _verify_structure_settings_v924(r)

    def test_anvil_damage(self):
        w = PacketWriter()
        w.write_byte(3)  # Damage
        _write_block_pos(w, -10, 50, 30)  # Position
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_anvil_damage(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_byte() == 3
        assert _read_net_block_pos(r) == (-10, 50, 30)
        assert not r.has_remaining
