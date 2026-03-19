"""Tests for the login handlers (RequestNetworkSettings protocol spoofing)."""

from __future__ import annotations

import struct
from unittest.mock import MagicMock

import pytest

from endstone_endweave.codec import PacketWrapper, INT_BE
from endstone_endweave.connection import UserConnection
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.v924_to_v944.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)


class TestRequestNetworkSettings:
    def setup_method(self):
        self.connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)

    def test_rewrites_944_to_924(self):
        payload = struct.pack(">i", 944)
        wrapper = PacketWrapper(payload, user=self.connection)
        rewrite_request_network_settings(wrapper)
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924

    def test_no_rewrite_when_matching(self):
        payload = struct.pack(">i", 924)
        wrapper = PacketWrapper(payload, user=self.connection)
        rewrite_request_network_settings(wrapper)
        assert wrapper.to_bytes() == payload

    def test_preserves_trailing_data(self):
        payload = struct.pack(">i", 944) + b"\xde\xad\xbe\xef"
        wrapper = PacketWrapper(payload, user=self.connection)
        rewrite_request_network_settings(wrapper)
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924
        assert result[4:] == b"\xde\xad\xbe\xef"


class TestLoginPacket:
    def setup_method(self):
        self.connection = UserConnection(address="1.2.3.4:1234", logger=MagicMock(), server_protocol=924)

    def test_rewrites_protocol_version(self):
        payload = struct.pack(">i", 944) + b"\x00" * 100
        wrapper = PacketWrapper(payload, user=self.connection)
        rewrite_login(wrapper)
        result = wrapper.to_bytes()
        assert struct.unpack(">i", result[:4])[0] == 924
        assert len(result) == len(payload)

    def test_no_rewrite_when_matching(self):
        payload = struct.pack(">i", 924) + b"\x00" * 100
        wrapper = PacketWrapper(payload, user=self.connection)
        rewrite_login(wrapper)
        assert wrapper.to_bytes() == payload


class TestV924ToV944Protocol:
    def test_cancels_new_serverbound_packets(self):
        from endstone_endweave.protocol.v924_to_v944.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), client_protocol=944, server_protocol=924
        )

        wrapper = PacketWrapper(b"\x00", user=connection)
        protocol.transform(Direction.SERVERBOUND, 343, wrapper)
        assert wrapper.cancelled

    def test_passthrough_normal_packets(self):
        from endstone_endweave.protocol.v924_to_v944.protocol import create_protocol

        protocol = create_protocol()
        connection = UserConnection(
            address="1.2.3.4:1234", logger=MagicMock(), client_protocol=944, server_protocol=924
        )

        wrapper = PacketWrapper(b"\x00\x01\x02", user=connection)
        protocol.transform(Direction.SERVERBOUND, 50, wrapper)
        assert not wrapper.cancelled
        assert wrapper.to_bytes() == b"\x00\x01\x02"
