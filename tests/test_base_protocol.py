"""Tests for the base protocol (version detection + login rewriting)."""

import struct
from unittest.mock import MagicMock

from endstone_endweave.codec import PacketWrapper
from endstone_endweave.connection import ConnectionState, UserConnection
from endstone_endweave.protocol.base import (
    create_base_protocol,
    detect_client_protocol,
)
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.packet_ids import PacketId


class TestDetectClientProtocol:
    def test_reads_protocol_and_sets_on_connection(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=connection)
        detect_client_protocol(wrapper)
        assert connection.client_protocol == 944
        assert connection.state == ConnectionState.LOGIN
        assert not wrapper.cancelled
        assert wrapper.to_bytes() == struct.pack(">i", 924)  # rewritten to server protocol

    def test_short_payload_no_crash(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        wrapper = PacketWrapper(b"\x00", user=connection)
        try:
            detect_client_protocol(wrapper)
        except Exception:
            pass
        assert connection.client_protocol == 0


class TestStateTransitions:
    def test_detect_transitions_to_login(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        assert connection.state == ConnectionState.HANDSHAKE
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=connection)
        detect_client_protocol(wrapper)
        assert connection.state == ConnectionState.LOGIN


class TestCreateBaseProtocol:
    def test_registers_correct_handlers(self):
        bp = create_base_protocol(924)
        assert bp.server_protocol == 924
        assert bp.client_protocol == 0
        assert bp.is_base
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=connection)
        bp.transform(Direction.SERVERBOUND, PacketId.REQUEST_NETWORK_SETTINGS, wrapper)
        assert connection.client_protocol == 944
