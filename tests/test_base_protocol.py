"""Tests for the base protocol (version detection + disconnect logging)."""

from __future__ import annotations

import struct
from unittest.mock import MagicMock

import pytest

from endstone_endweave.codec import PacketWrapper, UVAR_INT, BOOL, STRING
from endstone_endweave.codec.writer import PacketWriter
from endstone_endweave.connection import UserConnection
from endstone_endweave.protocol.base_protocol import (
    create_base_protocol,
    detect_client_protocol,
    log_disconnect,
)
from endstone_endweave.protocol.packet_ids import PacketId


class TestDetectClientProtocol:
    def test_reads_protocol_and_sets_on_connection(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload)
        detect_client_protocol(wrapper, connection)
        assert connection.client_protocol == 944
        assert not wrapper.cancelled
        # passthrough: output should match input
        assert wrapper.to_bytes() == payload

    def test_short_payload_no_crash(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        # INT_BE needs 4 bytes, only 1 provided -- should raise
        # The base protocol is only called for REQUEST_NETWORK_SETTINGS which
        # always has >= 4 bytes, but we test the wrapper still works
        wrapper = PacketWrapper(b"\x00")
        try:
            detect_client_protocol(wrapper, connection)
        except Exception:
            pass
        # client_protocol unchanged
        assert connection.client_protocol == 0


class TestLogDisconnect:
    def test_parses_disconnect_without_modifying(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        w = PacketWriter()
        w.write_uvarint(0)  # reason
        w.write_bool(False)  # skip_message = false
        w.write_string("kicked")
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        log_disconnect(wrapper, connection)
        assert not wrapper.cancelled
        # passthrough: output should match input
        assert wrapper.to_bytes() == payload

    def test_handles_malformed_payload(self):
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        wrapper = PacketWrapper(b"\xff\xff\xff\xff\xff\xff")
        log_disconnect(wrapper, connection)
        assert not wrapper.cancelled


class TestCreateBaseProtocol:
    def test_registers_correct_handlers(self):
        bp = create_base_protocol(924)
        assert bp.server_protocol == 924
        assert bp.client_protocol == 0
        # Verify handlers are registered by checking they produce results
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload)
        bp.transform_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, wrapper, connection)
        assert connection.client_protocol == 944

    def test_disconnect_handler_registered(self):
        bp = create_base_protocol(924)
        connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)
        w = PacketWriter()
        w.write_uvarint(0)
        w.write_bool(True)  # skip message
        payload = w.to_bytes()
        wrapper = PacketWrapper(payload)
        bp.transform_clientbound(PacketId.DISCONNECT, wrapper, connection)
        assert not wrapper.cancelled
