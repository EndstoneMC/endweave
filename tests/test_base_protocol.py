"""Tests for the base protocol (version detection + disconnect logging)."""

import struct
from unittest.mock import MagicMock


from endstone_endweave.codec import PacketWrapper
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.connection import ConnectionState, UserConnection
from endstone_endweave.protocol.base import (
    create_base_protocol,
    detect_client_protocol,
    log_disconnect,
    _transition_to_play,
)
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.packet_ids import PacketId


class TestDetectClientProtocol:
    def test_reads_protocol_and_sets_on_connection(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=connection)
        detect_client_protocol(wrapper)
        assert connection.client_protocol == 944
        assert connection.state == ConnectionState.LOGIN
        assert not wrapper.cancelled
        assert wrapper.to_bytes() == payload

    def test_short_payload_no_crash(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        wrapper = PacketWrapper(b"\x00", user=connection)
        try:
            detect_client_protocol(wrapper)
        except Exception:
            pass
        assert connection.client_protocol == 0


class TestLogDisconnect:
    def test_parses_disconnect_without_modifying(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        w = PacketWriter()
        w.write_uvarint(0)
        w.write_bool(False)
        w.write_string("kicked")
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload, user=connection)
        log_disconnect(wrapper)
        assert not wrapper.cancelled
        assert wrapper.to_bytes() == payload
        assert connection.pending_disconnect

    def test_handles_malformed_payload(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        wrapper = PacketWrapper(b"\xff\xff\xff\xff\xff\xff", user=connection)
        log_disconnect(wrapper)
        assert not wrapper.cancelled
        assert connection.pending_disconnect


class TestStateTransitions:
    def test_detect_transitions_to_login(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        assert connection.state == ConnectionState.HANDSHAKE
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=connection)
        detect_client_protocol(wrapper)
        assert connection.state == ConnectionState.LOGIN

    def test_start_game_transitions_to_play(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        connection.state = ConnectionState.LOGIN
        wrapper = PacketWrapper(b"\x00", user=connection)
        _transition_to_play(wrapper)
        assert connection.state == ConnectionState.PLAY

    def test_play_transition_idempotent(self):
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        connection.state = ConnectionState.PLAY
        wrapper = PacketWrapper(b"\x00", user=connection)
        _transition_to_play(wrapper)
        assert connection.state == ConnectionState.PLAY


class TestCreateBaseProtocol:
    def test_registers_correct_handlers(self):
        bp = create_base_protocol(924)
        assert bp.server_protocol == 924
        assert bp.client_protocol == 0
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=connection)
        bp.transform(Direction.SERVERBOUND, PacketId.REQUEST_NETWORK_SETTINGS, wrapper)
        assert connection.client_protocol == 944

    def test_disconnect_handler_registered(self):
        bp = create_base_protocol(924)
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924
        )
        w = PacketWriter()
        w.write_uvarint(0)
        w.write_bool(True)
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload, user=connection)
        bp.transform(Direction.CLIENTBOUND, PacketId.DISCONNECT, wrapper)
        assert not wrapper.cancelled
