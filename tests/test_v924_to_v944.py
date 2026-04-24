"""Tests for v924 (1.26.0) server <- v944 (1.26.10) client protocol translation."""

import struct
from unittest.mock import MagicMock

from helpers import (
    make_spline_v924,
    read_block_pos,
    read_net_block_pos,
    verify_structure_settings_v924,
    write_block_pos,
    write_net_block_pos,
    write_structure_settings_v944,
)

from endstone_endweave.codec import PacketReader, PacketWrapper
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.connection import UserConnection
from endstone_endweave.protocol.base import _rewrite_login, detect_client_protocol
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.v924_to_v944.handlers.block_pos import (
    rewrite_command_block_update,
    rewrite_inventory_transaction,
    rewrite_map_data,
    rewrite_structure_block_update,
    rewrite_structure_template_data_request,
    rewrite_tile_event,
    rewrite_update_client_input_locks,
    rewrite_update_sub_chunk_blocks,
)
from endstone_endweave.protocol.v924_to_v944.handlers.camera import (
    rewrite_camera_instruction,
    rewrite_camera_spline,
)
from endstone_endweave.protocol.v924_to_v944.handlers.start_game import (
    rewrite_start_game,
)

# ---------------------------------------------------------------------------
# Section 1: Protocol-level tests
# ---------------------------------------------------------------------------


class TestRequestNetworkSettings:
    def setup_method(self):
        self.connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)

    def test_rewrites_944_to_924(self):
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=self.connection)
        detect_client_protocol(wrapper)
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924

    def test_no_rewrite_when_matching(self):
        payload = struct.pack(">i", 924)
        wrapper = PacketWrapper(payload, user=self.connection)
        detect_client_protocol(wrapper)
        assert wrapper.to_bytes() == payload

    def test_preserves_trailing_data(self):
        payload = struct.pack(">i", 944) + b"\xde\xad\xbe\xef"
        wrapper = PacketWrapper(payload, user=self.connection)
        detect_client_protocol(wrapper)
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924
        assert result[4:] == b"\xde\xad\xbe\xef"


class TestLoginPacket:
    def setup_method(self):
        self.connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)

    def test_rewrites_protocol_version(self):
        payload = struct.pack(">i", 944) + b"\x00" * 100
        wrapper = PacketWrapper(payload, user=self.connection)
        _rewrite_login(wrapper)
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924
        assert len(result) == len(payload)

    def test_no_rewrite_when_matching(self):
        payload = struct.pack(">i", 924) + b"\x00" * 100
        wrapper = PacketWrapper(payload, user=self.connection)
        _rewrite_login(wrapper)
        assert wrapper.to_bytes() == payload


class TestV924ToV944Protocol:
    def test_cancels_new_serverbound_packets(self):
        from endstone_endweave.protocol.v924_to_v944.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=944,
            server_protocol=924,
        )

        wrapper = PacketWrapper(b"\x00", user=connection)
        protocol.transform(Direction.SERVERBOUND, 343, wrapper)
        assert wrapper.cancelled

    def test_cancels_editor_network_serverbound(self):
        from endstone_endweave.protocol.v924_to_v944.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=944,
            server_protocol=924,
        )

        wrapper = PacketWrapper(b"\x01\x04test\x04data", user=connection)
        protocol.transform(Direction.SERVERBOUND, 190, wrapper)
        assert wrapper.cancelled

    def test_cancels_editor_network_clientbound(self):
        from endstone_endweave.protocol.v924_to_v944.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=944,
            server_protocol=924,
        )

        wrapper = PacketWrapper(b"\x01\x04test\x04data", user=connection)
        protocol.transform(Direction.CLIENTBOUND, 190, wrapper)
        assert wrapper.cancelled

    def test_passthrough_normal_packets(self):
        from endstone_endweave.protocol.v924_to_v944.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=944,
            server_protocol=924,
        )

        wrapper = PacketWrapper(b"\x00\x01\x02", user=connection)
        protocol.transform(Direction.SERVERBOUND, 50, wrapper)
        assert not wrapper.cancelled
        assert wrapper.to_bytes() == b"\x00\x01\x02"


class TestV944ToV924Protocol:
    def test_cancels_editor_network_clientbound(self):
        from endstone_endweave.protocol.v944_to_v924.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=924,
            server_protocol=944,
        )

        wrapper = PacketWrapper(b"\x01\x04test\x04data", user=connection)
        protocol.transform(Direction.CLIENTBOUND, 190, wrapper)
        assert wrapper.cancelled

    def test_cancels_editor_network_serverbound(self):
        from endstone_endweave.protocol.v944_to_v924.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=924,
            server_protocol=944,
        )

        wrapper = PacketWrapper(b"\x01\x04test\x04data", user=connection)
        protocol.transform(Direction.SERVERBOUND, 190, wrapper)
        assert wrapper.cancelled


# ---------------------------------------------------------------------------
# Section 2: Complex handler tests
# ---------------------------------------------------------------------------


class TestUpdateSubChunkBlocks:
    def test_empty(self):
        w = PacketWriter()
        write_net_block_pos(w, 10, 64, 20)  # SubChunk position
        w.write_uvarint(0)  # blocks count = 0
        w.write_uvarint(0)  # extra count = 0
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_sub_chunk_blocks(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert read_block_pos(r) == (10, 64, 20)
        assert r.read_uvarint() == 0
        assert r.read_uvarint() == 0
        assert not r.has_remaining

    def test_with_entries(self):
        w = PacketWriter()
        write_net_block_pos(w, 0, 4, 0)  # SubChunk position
        # 2 block entries
        w.write_uvarint(2)
        for pos_y in [64, 65]:
            write_net_block_pos(w, 1, pos_y, 2)
            w.write_uvarint(100)  # blockRuntimeID
            w.write_uvarint(3)  # flags
            w.write_uvarint64(0)  # syncedUpdateEntityUniqueID
            w.write_uvarint(0)  # syncedUpdateType
        # 1 extra entry
        w.write_uvarint(1)
        write_net_block_pos(w, 3, 66, 4)
        w.write_uvarint(200)
        w.write_uvarint(1)
        w.write_uvarint64(99)
        w.write_uvarint(1)  # syncedUpdateType
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_sub_chunk_blocks(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert read_block_pos(r) == (0, 4, 0)
        assert r.read_uvarint() == 2
        assert read_block_pos(r) == (1, 64, 2)
        assert r.read_uvarint() == 100
        assert r.read_uvarint() == 3
        assert r.read_uvarint64() == 0
        assert r.read_uvarint() == 0  # syncedUpdateType
        assert read_block_pos(r) == (1, 65, 2)
        assert r.read_uvarint() == 100
        assert r.read_uvarint() == 3
        assert r.read_uvarint64() == 0
        assert r.read_uvarint() == 0  # syncedUpdateType
        assert r.read_uvarint() == 1
        assert read_block_pos(r) == (3, 66, 4)
        assert r.read_uvarint() == 200
        assert r.read_uvarint() == 1
        assert r.read_uvarint64() == 99
        assert r.read_uvarint() == 1  # syncedUpdateType
        assert not r.has_remaining


class TestUpdateClientInputLocks:
    def test_strips_server_pos(self):
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


class TestCommandBlockUpdate:
    def test_is_block(self):
        w = PacketWriter()
        w.write_bool(True)  # isBlock
        write_block_pos(w, 100, 64, 200)  # Position
        w.write_bytes(b"\xaa\xbb")  # rest of packet
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_command_block_update(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is True
        assert read_net_block_pos(r) == (100, 64, 200)
        assert r.read_remaining() == b"\xaa\xbb"

    def test_not_block(self):
        w = PacketWriter()
        w.write_bool(False)  # isBlock = false -> no BlockPos
        w.write_bytes(b"\xcc\xdd")
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_command_block_update(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is False
        assert r.read_remaining() == b"\xcc\xdd"


class TestStructureBlockUpdate:
    def test_converts_position_and_settings(self):
        w = PacketWriter()
        write_block_pos(w, 5, 100, -5)  # Block Position
        # StructureEditorData
        w.write_string("my_struct")  # Name
        w.write_string("")  # DataField
        w.write_bool(False)  # IncludePlayers
        w.write_bool(True)  # ShowBoundingBox
        w.write_varint(1)  # StructureBlockType
        write_structure_settings_v944(w)
        w.write_varint(0)  # RedstoneSaveMode
        # Remaining fields
        w.write_bool(True)  # Trigger
        w.write_bool(False)  # IsWaterlogged
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_structure_block_update(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert read_net_block_pos(r) == (5, 100, -5)
        assert r.read_string() == "my_struct"
        assert r.read_string() == ""
        assert r.read_bool() is False
        assert r.read_bool() is True
        assert r.read_varint() == 1
        verify_structure_settings_v924(r)


class TestStructureTemplateDataRequest:
    def test_converts_position_and_settings(self):
        w = PacketWriter()
        w.write_string("my_structure")  # Name
        write_block_pos(w, 0, 64, 0)  # Position
        write_structure_settings_v944(w)
        w.write_byte(1)  # RequestedOperation
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_structure_template_data_request(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_string() == "my_structure"
        assert read_net_block_pos(r) == (0, 64, 0)
        verify_structure_settings_v924(r)


# ---------------------------------------------------------------------------
# Section 3: New tests for coverage gaps
# ---------------------------------------------------------------------------


class TestTileEvent:
    def test_non_note_block(self):
        """event_type != 0: event_data unchanged."""
        w = PacketWriter()
        write_net_block_pos(w, 10, 64, 20)
        w.write_varint(1)  # Event Type (not note block)
        w.write_varint(20)  # Event Value
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_tile_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert read_block_pos(r) == (10, 64, 20)
        assert r.read_varint() == 1
        assert r.read_varint() == 20  # unchanged

    def test_note_block_below_trumpet(self):
        """event_type=0, event_data=15: below trumpet threshold, unchanged."""
        w = PacketWriter()
        write_net_block_pos(w, 0, 64, 0)
        w.write_varint(0)  # Event Type (note block)
        w.write_varint(15)  # Event Value (below 16)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_tile_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        read_block_pos(r)
        assert r.read_varint() == 0
        assert r.read_varint() == 15  # unchanged

    def test_note_block_at_trumpet(self):
        """event_type=0, event_data=16: at trumpet insertion point, shifts by +4."""
        w = PacketWriter()
        write_net_block_pos(w, 0, 64, 0)
        w.write_varint(0)  # Event Type
        w.write_varint(16)  # Event Value (at threshold)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_tile_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        read_block_pos(r)
        assert r.read_varint() == 0
        assert r.read_varint() == 20  # 16 + 4

    def test_note_block_above_trumpet(self):
        """event_type=0, event_data=20: above threshold, shifts by +4."""
        w = PacketWriter()
        write_net_block_pos(w, 0, 64, 0)
        w.write_varint(0)
        w.write_varint(20)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_tile_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        read_block_pos(r)
        assert r.read_varint() == 0
        assert r.read_varint() == 24  # 20 + 4


class TestMapData:
    def test_no_flags(self):
        """types=0: minimal passthrough, no decoration/creation/texture."""
        w = PacketWriter()
        w.write_varint64(1)  # Map ID
        w.write_uvarint(0)  # Type Flags
        w.write_byte(0)  # Dimension
        w.write_bool(False)  # Is Locked Map?
        # Map Origin (v924 sends BlockPos here -- it's not a NetworkBlockPos in the map packet)
        w.write_varint(0)
        w.write_varint(64)
        w.write_varint(0)
        w.write_bytes(b"\xff")  # trailing
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_map_data(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_varint64() == 1
        assert r.read_uvarint() == 0
        assert r.read_byte() == 0
        assert r.read_bool() is False
        assert read_block_pos(r) == (0, 64, 0)

    def test_creation_flag(self):
        """types=0x08: has MapIDList."""
        w = PacketWriter()
        w.write_varint64(1)
        w.write_uvarint(0x08)  # CREATION
        w.write_byte(0)
        w.write_bool(False)
        w.write_varint(0)
        w.write_varint(0)
        w.write_varint(0)  # Map Origin
        w.write_uvarint(2)  # MapIDList count
        w.write_varint64(10)
        w.write_varint64(20)
        w.write_byte(1)  # Scale
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_map_data(wrapper)
        r = PacketReader(wrapper.to_bytes())
        r.read_varint64()  # Map ID
        assert r.read_uvarint() == 0x08
        r.read_byte()
        r.read_bool()
        read_block_pos(r)
        assert r.read_uvarint() == 2
        assert r.read_varint64() == 10
        assert r.read_varint64() == 20
        assert r.read_byte() == 1  # Scale

    def test_decoration_entity(self):
        """types=0x04: decoration with entity object (type=0)."""
        w = PacketWriter()
        w.write_varint64(1)
        w.write_uvarint(0x04)  # DECORATION
        w.write_byte(0)
        w.write_bool(False)
        w.write_varint(0)
        w.write_varint(0)
        w.write_varint(0)
        w.write_byte(1)  # Scale
        w.write_uvarint(1)  # 1 tracked object
        w.write_int_le(0)  # Type = Entity
        w.write_varint64(42)  # UniqueId
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_map_data(wrapper)
        r = PacketReader(wrapper.to_bytes())
        r.read_varint64()
        r.read_uvarint()
        r.read_byte()
        r.read_bool()
        read_block_pos(r)
        r.read_byte()  # Scale
        assert r.read_uvarint() == 1
        assert r.read_int_le() == 0
        assert r.read_varint64() == 42

    def test_decoration_block(self):
        """types=0x04: decoration with block object (type=1), NetworkBlockPos converted."""
        w = PacketWriter()
        w.write_varint64(1)
        w.write_uvarint(0x04)  # DECORATION
        w.write_byte(0)
        w.write_bool(False)
        w.write_varint(0)
        w.write_varint(0)
        w.write_varint(0)
        w.write_byte(1)  # Scale
        w.write_uvarint(1)  # 1 tracked object
        w.write_int_le(1)  # Type = Block
        write_net_block_pos(w, 100, 64, -200)  # NetworkBlockPos
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_map_data(wrapper)
        r = PacketReader(wrapper.to_bytes())
        r.read_varint64()
        r.read_uvarint()
        r.read_byte()
        r.read_bool()
        read_block_pos(r)
        r.read_byte()  # Scale
        assert r.read_uvarint() == 1
        assert r.read_int_le() == 1
        assert read_block_pos(r) == (100, 64, -200)


class TestInventoryTransaction:
    def test_non_use_item_passthrough(self):
        """transaction_type != 2: remaining bytes passed through unchanged."""
        w = PacketWriter()
        w.write_varint(0)  # legacy_request_id = 0
        w.write_uvarint(0)  # transaction_type = Normal (not UseItem)
        w.write_uvarint(0)  # action_count = 0
        w.write_bytes(b"\xaa\xbb")  # trailing data
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_inventory_transaction(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_varint() == 0
        assert r.read_uvarint() == 0
        assert r.read_uvarint() == 0
        assert r.read_remaining() == b"\xaa\xbb"

    def test_use_item(self):
        """transaction_type=2 (UseItem): BlockPos converted, ClientCooldownState stripped."""
        w = PacketWriter()
        w.write_varint(0)  # legacy_request_id
        w.write_uvarint(2)  # transaction_type = UseItem
        w.write_uvarint(0)  # action_count = 0
        # UseItemTransactionData
        w.write_uvarint(0)  # ActionType
        w.write_uvarint(0)  # TriggerType
        write_block_pos(w, 10, -5, 20)  # BlockPosition (v944 BlockPos)
        w.write_varint(1)  # BlockFace
        w.write_varint(0)  # HotBarSlot
        # HeldItem = air (network_id=0)
        w.write_varint(0)
        # Position
        w.write_float_le(10.5)
        w.write_float_le(-4.5)
        w.write_float_le(20.5)
        # ClickedPosition
        w.write_float_le(0.5)
        w.write_float_le(1.0)
        w.write_float_le(0.5)
        w.write_uvarint(42)  # BlockRuntimeID
        w.write_uvarint(0)  # ClientPrediction
        w.write_byte(1)  # ClientCooldownState (to be stripped)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_inventory_transaction(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_varint() == 0  # legacy_request_id
        assert r.read_uvarint() == 2  # transaction_type
        assert r.read_uvarint() == 0  # action_count
        assert r.read_uvarint() == 0  # ActionType
        assert r.read_uvarint() == 0  # TriggerType
        assert read_net_block_pos(r) == (10, -5 & 0xFFFFFFFF, 20)  # converted
        assert r.read_varint() == 1  # BlockFace
        assert r.read_varint() == 0  # HotBarSlot
        assert r.read_varint() == 0  # HeldItem (air)
        # Position
        r.read_float_le()
        r.read_float_le()
        r.read_float_le()
        # ClickedPosition
        r.read_float_le()
        r.read_float_le()
        r.read_float_le()
        assert r.read_uvarint() == 42  # BlockRuntimeID
        assert r.read_uvarint() == 0  # ClientPrediction
        assert not r.has_remaining  # ClientCooldownState stripped

    def test_with_legacy_request(self):
        """legacy_request_id != 0: legacy slot data passed through."""
        w = PacketWriter()
        w.write_varint(1)  # legacy_request_id != 0
        # Legacy Set Item Slots
        w.write_uvarint(1)  # slot_count = 1
        w.write_byte(5)  # Container Enum
        w.write_uvarint(2)  # slots_len = 2
        w.write_byte(0)  # Slot 0
        w.write_byte(1)  # Slot 1
        w.write_uvarint(0)  # transaction_type = Normal (not UseItem)
        w.write_uvarint(0)  # action_count = 0
        w.write_bytes(b"\xcc")  # trailing
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_inventory_transaction(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_varint() == 1
        assert r.read_uvarint() == 1  # slot_count
        assert r.read_byte() == 5  # Container Enum
        assert r.read_uvarint() == 2  # slots_len
        assert r.read_byte() == 0
        assert r.read_byte() == 1
        assert r.read_uvarint() == 0  # transaction_type
        assert r.read_uvarint() == 0  # action_count
        assert r.read_remaining() == b"\xcc"


class TestCameraInstruction:
    def test_all_empty(self):
        """All optional sections absent."""
        w = PacketWriter()
        w.write_bool(False)  # Set
        w.write_bool(False)  # Clear: not present
        w.write_bool(False)  # Fade
        w.write_bool(False)  # Target
        w.write_bool(False)  # RemoveTarget: not present
        w.write_bool(False)  # FieldOfView
        w.write_bool(False)  # Spline
        # AttachToEntity (OptionalType(INT64_LE)): not present
        w.write_bool(False)
        # DetachFromEntity (OPTIONAL_BOOL): not present
        w.write_bool(False)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_instruction(wrapper)
        assert wrapper.to_bytes() == w.to_bytes()  # unchanged

    def test_with_set_and_ease(self):
        """Set instruction with EaseOption present."""
        w = PacketWriter()
        w.write_bool(True)  # Set: present
        w.write_int_le(1)  # preset
        w.write_bool(True)  # has ease
        w.write_byte(2)  # ease type
        w.write_float_le(1.5)  # ease time
        # pos: not present
        w.write_bool(False)
        # rot: not present
        w.write_bool(False)
        # facing: not present
        w.write_bool(False)
        # view_offset: not present
        w.write_bool(False)
        # entity_offset: not present
        w.write_bool(False)
        # default: not present
        w.write_bool(False)
        w.write_bool(False)  # removeIgnoreStartingValuesComponent
        w.write_bool(False)  # Clear
        w.write_bool(False)  # Fade
        w.write_bool(False)  # Target
        w.write_bool(False)  # RemoveTarget
        w.write_bool(False)  # FieldOfView
        w.write_bool(False)  # Spline
        w.write_bool(False)  # AttachToEntity
        w.write_bool(False)  # DetachFromEntity
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_instruction(wrapper)
        assert wrapper.to_bytes() == w.to_bytes()

    def test_with_fade_time_and_color(self):
        """Fade with Time and Color options."""
        w = PacketWriter()
        w.write_bool(False)  # Set
        w.write_bool(False)  # Clear
        w.write_bool(True)  # Fade: present
        w.write_bool(True)  # has Time
        w.write_float_le(0.5)  # Fade In
        w.write_float_le(1.0)  # Hold
        w.write_float_le(0.5)  # Fade Out
        w.write_bool(True)  # has Color
        w.write_float_le(0.0)  # Red
        w.write_float_le(0.0)  # Green
        w.write_float_le(0.0)  # Blue
        w.write_bool(False)  # Target
        w.write_bool(False)  # RemoveTarget
        w.write_bool(False)  # FieldOfView
        w.write_bool(False)  # Spline
        w.write_bool(False)  # AttachToEntity
        w.write_bool(False)  # DetachFromEntity
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_instruction(wrapper)
        assert wrapper.to_bytes() == w.to_bytes()

    def test_with_spline(self):
        """Spline present: v924 SplineInstruction mapped to v944 format."""
        spline_bytes = make_spline_v924(total_time=3.0)
        w = PacketWriter()
        w.write_bool(False)  # Set
        w.write_bool(False)  # Clear
        w.write_bool(False)  # Fade
        w.write_bool(False)  # Target
        w.write_bool(False)  # RemoveTarget
        w.write_bool(False)  # FieldOfView
        w.write_bool(True)  # Spline: present
        w.write_bytes(spline_bytes)  # v924 SplineInstruction
        w.write_bool(False)  # AttachToEntity
        w.write_bool(False)  # DetachFromEntity
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_instruction(wrapper)
        result = wrapper.to_bytes()
        # v944 output should be longer (added splineIdentifier="" + loadFromJson=false)
        assert len(result) > len(w.to_bytes())

    def test_all_present(self):
        """All optional sections present."""
        spline_bytes = make_spline_v924()
        w = PacketWriter()
        # Set
        w.write_bool(True)
        w.write_int_le(0)  # preset
        w.write_bool(False)  # no ease
        w.write_bool(False)  # no pos
        w.write_bool(False)  # no rot
        w.write_bool(False)  # no facing
        w.write_bool(False)  # no view_offset
        w.write_bool(False)  # no entity_offset
        w.write_bool(False)  # no default
        w.write_bool(False)  # removeIgnoreStartingValues
        # Clear
        w.write_bool(True)  # present
        w.write_bool(False)  # value
        # Fade
        w.write_bool(True)
        w.write_bool(False)  # no Time
        w.write_bool(False)  # no Color
        # Target
        w.write_bool(True)
        w.write_bool(False)  # no Target Center Offset
        w.write_int64_le(99)  # Target Actor ID
        # RemoveTarget
        w.write_bool(True)
        w.write_bool(False)
        # FieldOfView
        w.write_bool(True)
        w.write_float_le(90.0)
        w.write_float_le(0.5)
        w.write_byte(0)
        w.write_bool(False)
        # Spline
        w.write_bool(True)
        w.write_bytes(spline_bytes)
        # AttachToEntity
        w.write_bool(True)
        w.write_int64_le(42)
        # DetachFromEntity
        w.write_bool(True)
        w.write_bool(True)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        rewrite_camera_instruction(wrapper)
        result = wrapper.to_bytes()
        # v944 output should be longer (added splineIdentifier="" + loadFromJson=false)
        assert len(result) > len(payload)


class TestCameraSpline:
    def test_zero_splines(self):
        w = PacketWriter()
        w.write_uvarint(0)  # count = 0
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_spline(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == 0
        assert not r.has_remaining

    def test_with_entries(self):
        """Spline entries get SplineIdentifier and LoadFromJson appended."""
        spline_bytes = make_spline_v924(total_time=2.0)
        w = PacketWriter()
        w.write_uvarint(1)  # 1 spline definition
        w.write_string("test_spline")  # name
        w.write_bytes(spline_bytes)  # v924 SplineInstruction
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_camera_spline(wrapper)
        result = wrapper.to_bytes()
        r = PacketReader(result)
        assert r.read_uvarint() == 1
        assert r.read_string() == "test_spline"
        # v944 SplineInstruction should have the original fields plus 2 new ones
        r.read_float_le()  # totalTime
        r.read_byte()  # type
        count = r.read_uvarint()  # curve count
        for _ in range(count):
            r.read_float_le()
            r.read_float_le()
            r.read_float_le()
        r.read_uvarint()  # progress key frames count (0)
        r.read_uvarint()  # rotation key frames count (0)
        # v944 additions
        assert r.read_string() == ""  # SplineIdentifier (default)
        assert r.read_bool() is False  # LoadFromJson (default)
        assert not r.has_remaining


# ---------------------------------------------------------------------------
# StartGamePacket
# ---------------------------------------------------------------------------


def _build_v924_start_game(has_server_join_info: bool, has_gathering: bool = False) -> bytes:
    """Build a synthetic v924 StartGamePacket payload."""
    w = PacketWriter()

    w.write_varint64(1)  # mEntityId
    w.write_uvarint64(2)  # mRuntimeId
    w.write_varint(0)  # mEntityGameType (survival)
    w.write_float_le(1.0)  # mPos.X
    w.write_float_le(64.0)  # mPos.Y
    w.write_float_le(1.0)  # mPos.Z
    w.write_float_le(0.0)  # mRot.X
    w.write_float_le(0.0)  # mRot.Y
    w.write_int64_le(12345)  # seed
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

    w.write_string("level-id")  # level ID
    w.write_string("My World")  # level name
    w.write_string("")  # template content identity
    w.write_bool(False)  # is trial
    w.write_varint(0)  # rewind history size
    w.write_bool(True)  # server auth block breaking
    w.write_int64_le(1000)  # level current time
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

    w.write_int64_le(0)  # block registry checksum
    w.write_int64_le(0)  # world template MSB
    w.write_int64_le(0)  # world template LSB
    w.write_bool(False)  # clientside generation
    w.write_bool(True)  # block network ids are hashes
    w.write_bool(False)  # server auth sound

    # Server join info (v924 format)
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

        r = PacketReader(result)
        assert r.read_varint64() == 1  # entity id
        assert r.read_uvarint64() == 2  # runtime id
        r.read_varint()  # game type
        for _ in range(4):
            r.read_float_le()
        r.read_float_le()  # rot Y
        r.read_int64_le()  # seed
        r.read_short_le()
        r.read_string()
        r.read_varint()
        r.read_varint()
        r.read_varint()
        r.read_bool()
        r.read_varint()

        # DefaultSpawn -- Y should now be varint (v944 format)
        x = r.read_varint()
        y = r.read_varint()  # was uvarint, now varint
        z = r.read_varint()
        assert x == 0
        assert y == 64
        assert z == 0

    def test_with_join_info_stripped(self):
        """v924 gathering data is stripped; v944 sub-fields are written."""
        payload_with_gather = _build_v924_start_game(has_server_join_info=True, has_gathering=True)
        payload_no_gather = _build_v924_start_game(has_server_join_info=True, has_gathering=False)
        assert len(payload_with_gather) > len(payload_no_gather)

        wrapper_gather = PacketWrapper(payload_with_gather)
        rewrite_start_game(wrapper_gather)

        wrapper_no_gather = PacketWrapper(payload_no_gather)
        rewrite_start_game(wrapper_no_gather)

        assert wrapper_gather.to_bytes() == wrapper_no_gather.to_bytes()

    def test_trailing_strings_preserved(self):
        """Verify the 4 trailing strings survive the rewrite."""
        payload = _build_v924_start_game(has_server_join_info=False)
        wrapper = PacketWrapper(payload)
        rewrite_start_game(wrapper)
        result = wrapper.to_bytes()

        assert b"server-id" in result
        assert b"scenario-id" in result
        assert b"world-id" in result
        assert b"owner-id" in result

    def test_all_variants_produce_valid_output(self):
        """Ensure all server join info variants produce valid to_bytes() output."""
        for has_join in [False, True]:
            for has_gather in [False, True]:
                if not has_join and has_gather:
                    continue
                payload = _build_v924_start_game(has_join, has_gather)
                wrapper = PacketWrapper(payload)
                rewrite_start_game(wrapper)
                result = wrapper.to_bytes()
                assert len(result) > 0, f"Empty output (join={has_join}, gather={has_gather})"
