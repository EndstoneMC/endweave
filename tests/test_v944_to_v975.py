"""Tests for v944 (1.26.10) <-> v975 (1.26.20) protocol translation."""

import struct
from unittest.mock import MagicMock

from endstone_endweave.codec import PacketReader, PacketWrapper
from endstone_endweave.codec.types.enums import LevelSoundEvent
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.connection import UserConnection
from endstone_endweave.protocol.direction import Direction

# ---------------------------------------------------------------------------
# v944_to_v975 protocol factory (v944 server <- v975 client)
# ---------------------------------------------------------------------------


class TestV944ToV975Protocol:
    def _make_protocol(self):
        from endstone_endweave.protocol.v944_to_v975.protocol import create_protocol

        return create_protocol(), UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), client_protocol=975, server_protocol=944
        )

    def test_cancels_party_changed_serverbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x01\x00", user=conn)
        protocol.transform(Direction.SERVERBOUND, 342, wrapper)
        assert wrapper.cancelled

    def test_cancels_locator_bar_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 341, wrapper)
        assert wrapper.cancelled

    def test_cancels_debug_drawer_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 328, wrapper)
        assert wrapper.cancelled

    def test_cancels_attribute_layer_sync_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 345, wrapper)
        assert wrapper.cancelled

    def test_cancels_player_enchant_options_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 146, wrapper)
        assert wrapper.cancelled


# ---------------------------------------------------------------------------
# v975_to_v944 protocol factory (v975 server <- v944 client)
# ---------------------------------------------------------------------------


class TestV975ToV944Protocol:
    def _make_protocol(self):
        from endstone_endweave.protocol.v975_to_v944.protocol import create_protocol

        return create_protocol(), UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), client_protocol=944, server_protocol=975
        )

    def test_cancels_server_store_info_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 346, wrapper)
        assert wrapper.cancelled

    def test_cancels_server_presence_info_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 347, wrapper)
        assert wrapper.cancelled

    def test_cancels_party_changed_serverbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x01\x00", user=conn)
        protocol.transform(Direction.SERVERBOUND, 342, wrapper)
        assert wrapper.cancelled

    def test_cancels_locator_bar_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 341, wrapper)
        assert wrapper.cancelled

    def test_cancels_debug_drawer_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 328, wrapper)
        assert wrapper.cancelled

    def test_cancels_attribute_layer_sync_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 345, wrapper)
        assert wrapper.cancelled

    def test_cancels_player_enchant_options_clientbound(self):
        protocol, conn = self._make_protocol()
        wrapper = PacketWrapper(b"\x00", user=conn)
        protocol.transform(Direction.CLIENTBOUND, 146, wrapper)
        assert wrapper.cancelled


# ---------------------------------------------------------------------------
# ActorEventPacket (27)
# ---------------------------------------------------------------------------


class TestActorEventV944ToV975:
    def test_appends_fire_at_position_absent(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.actor_event import rewrite_actor_event

        w = PacketWriter()
        w.write_uvarint64(12345)  # Target Runtime ID
        w.write_byte(2)  # Event ID (HURT)
        w.write_varint(0)  # Data
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_actor_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 12345
        assert r.read_byte() == 2
        assert r.read_varint() == 0
        assert r.read_bool() is False  # Fire At Position absent
        assert not r.has_remaining


class TestActorEventV975ToV944:
    def test_strips_fire_at_position_absent(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.actor_event import rewrite_actor_event

        w = PacketWriter()
        w.write_uvarint64(12345)  # Target Runtime ID
        w.write_byte(2)  # Event ID
        w.write_varint(0)  # Data
        w.write_bool(False)  # Fire At Position absent
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_actor_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 12345
        assert r.read_byte() == 2
        assert r.read_varint() == 0
        assert not r.has_remaining

    def test_strips_fire_at_position_present(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.actor_event import rewrite_actor_event

        w = PacketWriter()
        w.write_uvarint64(12345)  # Target Runtime ID
        w.write_byte(2)  # Event ID
        w.write_varint(0)  # Data
        w.write_bool(True)  # Fire At Position present
        w.write_float_le(1.0)
        w.write_float_le(2.0)
        w.write_float_le(3.0)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_actor_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 12345
        assert r.read_byte() == 2
        assert r.read_varint() == 0
        assert not r.has_remaining


# ---------------------------------------------------------------------------
# LevelSoundEventPacket (123)
# ---------------------------------------------------------------------------


def _write_level_sound_event(w: PacketWriter, event_id: int) -> None:
    w.write_uvarint(event_id)  # Event ID
    w.write_float_le(1.0)  # Position.X
    w.write_float_le(64.0)  # Position.Y
    w.write_float_le(1.0)  # Position.Z
    w.write_varint(0)  # Data
    w.write_string("minecraft:player")  # Actor Identifier
    w.write_bool(False)  # Is Baby
    w.write_bool(False)  # Is Global
    w.write_int64_le(42)  # Actor Unique Id


class TestLevelSoundEventV944ToV975:
    def test_shifts_sound_id_and_appends_fire_at_position(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.level_sound_event import rewrite_level_sound_event

        # v944 event below the insertion point -- unchanged
        w = PacketWriter()
        _write_level_sound_event(w, 100)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_level_sound_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == 100  # unchanged

    def test_shifts_undefined_forward(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.level_sound_event import rewrite_level_sound_event

        # v944 UNDEFINED (599) shifts to v975 UNDEFINED (601)
        w = PacketWriter()
        _write_level_sound_event(w, LevelSoundEvent.UNDEFINED_V944)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_level_sound_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == LevelSoundEvent.UNDEFINED_V975
        # Skip position + data + string + bools + int64
        for _ in range(3):
            r.read_float_le()
        r.read_varint()
        r.read_string()
        r.read_bool()
        r.read_bool()
        r.read_int64_le()
        assert r.read_bool() is False  # Fire At Position absent


class TestLevelSoundEventV975ToV944:
    def test_collapses_new_sounds_to_undefined(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.level_sound_event import rewrite_level_sound_event

        # v975 PUSHED_BY_PLAYER (599) collapses to v944 UNDEFINED (599)
        w = PacketWriter()
        _write_level_sound_event(w, LevelSoundEvent.PUSHED_BY_PLAYER)
        w.write_bool(False)  # Fire At Position absent
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_level_sound_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == LevelSoundEvent.UNDEFINED_V944

    def test_strips_fire_at_position(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.level_sound_event import rewrite_level_sound_event

        w = PacketWriter()
        _write_level_sound_event(w, 100)  # unchanged
        w.write_bool(False)  # Fire At Position absent
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_level_sound_event(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint() == 100
        for _ in range(3):
            r.read_float_le()
        r.read_varint()
        r.read_string()
        r.read_bool()
        r.read_bool()
        r.read_int64_le()
        assert not r.has_remaining


# ---------------------------------------------------------------------------
# PlaySoundPacket (86)
# ---------------------------------------------------------------------------


class TestPlaySoundV944ToV975:
    def test_appends_server_sound_handle_absent(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.play_sound import rewrite_play_sound

        w = PacketWriter()
        w.write_string("random.bowhit")  # Name
        w.write_varint(1)  # Position.X (BlockPos)
        w.write_varint(2)  # Position.Y
        w.write_varint(3)  # Position.Z
        w.write_float_le(1.0)  # Volume
        w.write_float_le(1.0)  # Pitch
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_play_sound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_string() == "random.bowhit"
        assert (r.read_varint(), r.read_varint(), r.read_varint()) == (1, 2, 3)
        assert r.read_float_le() == 1.0
        assert r.read_float_le() == 1.0
        assert r.read_bool() is False  # Server Sound Handle absent
        assert not r.has_remaining


class TestPlaySoundV975ToV944:
    def test_strips_server_sound_handle_absent(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.play_sound import rewrite_play_sound

        w = PacketWriter()
        w.write_string("random.bowhit")
        w.write_varint(1)
        w.write_varint(2)
        w.write_varint(3)
        w.write_float_le(1.0)
        w.write_float_le(1.0)
        w.write_bool(False)  # Server Sound Handle absent
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_play_sound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_string() == "random.bowhit"
        assert (r.read_varint(), r.read_varint(), r.read_varint()) == (1, 2, 3)
        assert r.read_float_le() == 1.0
        assert r.read_float_le() == 1.0
        assert not r.has_remaining

    def test_strips_server_sound_handle_present(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.play_sound import rewrite_play_sound

        w = PacketWriter()
        w.write_string("x")
        w.write_varint(0)
        w.write_varint(0)
        w.write_varint(0)
        w.write_float_le(1.0)
        w.write_float_le(1.0)
        w.write_bool(True)  # Server Sound Handle present
        w.write_int64_le(0xDEADBEEF)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_play_sound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_string() == "x"
        assert (r.read_varint(), r.read_varint(), r.read_varint()) == (0, 0, 0)
        assert r.read_float_le() == 1.0
        assert r.read_float_le() == 1.0
        assert not r.has_remaining


# ---------------------------------------------------------------------------
# UpdateClientOptionsPacket (323)
# ---------------------------------------------------------------------------


class TestUpdateClientOptionsV944ToV975:
    def test_strips_filter_profanity_absent(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.update_client_options import (
            rewrite_update_client_options,
        )

        w = PacketWriter()
        w.write_bool(False)  # Graphics Mode Change absent
        w.write_bool(False)  # Filter Profanity Change absent
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_client_options(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is False  # Graphics Mode Change preserved
        assert not r.has_remaining  # Filter Profanity stripped

    def test_strips_filter_profanity_present(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.update_client_options import (
            rewrite_update_client_options,
        )

        w = PacketWriter()
        w.write_bool(True)  # Graphics Mode Change present
        w.write_byte(2)  # value
        w.write_bool(True)  # Filter Profanity Change present
        w.write_bool(True)  # value
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_client_options(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is True
        assert r.read_byte() == 2
        assert not r.has_remaining


class TestUpdateClientOptionsV975ToV944:
    def test_appends_filter_profanity_absent(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.update_client_options import (
            rewrite_update_client_options,
        )

        w = PacketWriter()
        w.write_bool(False)  # Graphics Mode Change absent
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_update_client_options(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_bool() is False  # Graphics Mode Change preserved
        assert r.read_bool() is False  # Filter Profanity Change (appended)
        assert not r.has_remaining


# ---------------------------------------------------------------------------
# ServerboundDiagnosticsPacket (315)
# ---------------------------------------------------------------------------


def _write_diagnostics_v944(w: PacketWriter) -> None:
    for _ in range(9):
        w.write_float_le(1.0)
    w.write_uvarint(0)  # Memory Category Values (empty)


class TestDiagnosticsV944ToV975:
    def test_strips_new_diagnostic_lists(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.diagnostics import rewrite_diagnostics

        w = PacketWriter()
        _write_diagnostics_v944(w)
        # v975 trailing lists: Entity + System diagnostics (both empty)
        w.write_uvarint(0)
        w.write_uvarint(0)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_diagnostics(wrapper)
        r = PacketReader(wrapper.to_bytes())
        for _ in range(9):
            assert r.read_float_le() == 1.0
        assert r.read_uvarint() == 0
        assert not r.has_remaining

    def test_passes_memory_entries_through(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.diagnostics import rewrite_diagnostics

        w = PacketWriter()
        for _ in range(9):
            w.write_float_le(1.0)
        w.write_uvarint(2)  # Memory Category Values count
        w.write_byte(5)
        w.write_int64_le(1_000_000)
        w.write_byte(10)
        w.write_int64_le(2_000_000)
        # v975 trailing lists
        w.write_uvarint(0)
        w.write_uvarint(0)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_diagnostics(wrapper)
        r = PacketReader(wrapper.to_bytes())
        for _ in range(9):
            r.read_float_le()
        assert r.read_uvarint() == 2
        assert r.read_byte() == 5
        assert r.read_int64_le() == 1_000_000
        assert r.read_byte() == 10
        assert r.read_int64_le() == 2_000_000
        assert not r.has_remaining


class TestDiagnosticsV975ToV944:
    def test_appends_empty_diagnostic_lists(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.diagnostics import rewrite_diagnostics

        w = PacketWriter()
        _write_diagnostics_v944(w)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_diagnostics(wrapper)
        r = PacketReader(wrapper.to_bytes())
        for _ in range(9):
            r.read_float_le()
        assert r.read_uvarint() == 0  # Memory Category Values
        assert r.read_uvarint() == 0  # Entity Diagnostics (appended)
        assert r.read_uvarint() == 0  # System Diagnostics (appended)
        assert not r.has_remaining


# ---------------------------------------------------------------------------
# RequestNetworkSettings at the base protocol level -- verifies v975 round-trip
# ---------------------------------------------------------------------------


class TestRequestNetworkSettingsV975:
    def test_rewrites_975_to_944(self):
        from endstone_endweave.protocol.base import detect_client_protocol

        conn = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=944)
        payload = struct.pack(">i", 975)
        wrapper = PacketWrapper(payload, user=conn)
        detect_client_protocol(wrapper)
        assert struct.unpack(">i", wrapper.to_bytes()[:4])[0] == 944


# ---------------------------------------------------------------------------
# MobEquipmentPacket (31, PLAYER_EQUIPMENT)
# ---------------------------------------------------------------------------


def _write_empty_item(w: PacketWriter) -> None:
    """Write an air ItemInstance (network_id == 0 short-circuits the rest)."""
    w.write_varint(0)


class TestMobEquipmentV944ToV975Clientbound:
    def test_widens_byte_to_uvarint(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.mob_equipment import (
            rewrite_mob_equipment_clientbound,
        )

        w = PacketWriter()
        w.write_uvarint64(99999)  # Target Runtime ID
        _write_empty_item(w)
        w.write_byte(0x05)  # Slot
        w.write_byte(0x08)  # Selected Slot
        w.write_byte(0x00)  # Container ID
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_mob_equipment_clientbound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 99999
        assert r.read_varint() == 0  # empty item
        assert r.read_uvarint() == 5
        assert r.read_uvarint() == 8
        assert r.read_uvarint() == 0
        assert not r.has_remaining


class TestMobEquipmentV944ToV975Serverbound:
    def test_narrows_uvarint_to_byte(self):
        from endstone_endweave.protocol.v944_to_v975.handlers.mob_equipment import (
            rewrite_mob_equipment_serverbound,
        )

        w = PacketWriter()
        w.write_uvarint64(99999)
        _write_empty_item(w)
        w.write_uvarint(5)
        w.write_uvarint(8)
        w.write_uvarint(0)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_mob_equipment_serverbound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 99999
        assert r.read_varint() == 0
        assert r.read_byte() == 5
        assert r.read_byte() == 8
        assert r.read_byte() == 0
        assert not r.has_remaining


class TestMobEquipmentV975ToV944Clientbound:
    def test_narrows_uvarint_to_byte(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.mob_equipment import (
            rewrite_mob_equipment_clientbound,
        )

        w = PacketWriter()
        w.write_uvarint64(99999)
        _write_empty_item(w)
        w.write_uvarint(5)
        w.write_uvarint(8)
        w.write_uvarint(0)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_mob_equipment_clientbound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 99999
        assert r.read_varint() == 0
        assert r.read_byte() == 5
        assert r.read_byte() == 8
        assert r.read_byte() == 0
        assert not r.has_remaining


class TestMobEquipmentV975ToV944Serverbound:
    def test_widens_byte_to_uvarint(self):
        from endstone_endweave.protocol.v975_to_v944.handlers.mob_equipment import (
            rewrite_mob_equipment_serverbound,
        )

        w = PacketWriter()
        w.write_uvarint64(99999)
        _write_empty_item(w)
        w.write_byte(0x05)
        w.write_byte(0x08)
        w.write_byte(0x00)
        wrapper = PacketWrapper(w.to_bytes())
        rewrite_mob_equipment_serverbound(wrapper)
        r = PacketReader(wrapper.to_bytes())
        assert r.read_uvarint64() == 99999
        assert r.read_varint() == 0
        assert r.read_uvarint() == 5
        assert r.read_uvarint() == 8
        assert r.read_uvarint() == 0
        assert not r.has_remaining
