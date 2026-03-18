"""Tests for the login handlers (RequestNetworkSettings protocol spoofing)."""

from __future__ import annotations

import struct

import pytest

from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.v924_to_v944.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)


class TestRequestNetworkSettings:
    def setup_method(self):
        self.session = PlayerSession(address="1.2.3.4:1234", server_protocol=924)

    def test_rewrites_944_to_924(self):
        payload = struct.pack(">i", 944)
        result = rewrite_request_network_settings(payload, self.session)
        assert result.new_payload is not None
        assert struct.unpack(">i", result.new_payload[:4])[0] == 924
        assert self.session.client_protocol == 944

    def test_no_rewrite_when_matching(self):
        payload = struct.pack(">i", 924)
        result = rewrite_request_network_settings(payload, self.session)
        assert result.new_payload is None
        assert not result.cancel

    def test_preserves_trailing_data(self):
        payload = struct.pack(">i", 944) + b"\xde\xad\xbe\xef"
        result = rewrite_request_network_settings(payload, self.session)
        assert result.new_payload is not None
        assert result.new_payload[4:] == b"\xde\xad\xbe\xef"

    def test_short_payload_passthrough(self):
        result = rewrite_request_network_settings(b"\x00", self.session)
        assert result.new_payload is None
        assert not result.cancel


class TestLoginPacket:
    def setup_method(self):
        self.session = PlayerSession(address="1.2.3.4:1234", server_protocol=924)

    def test_rewrites_protocol_version(self):
        payload = struct.pack(">i", 944) + b"\x00" * 100  # JWT data
        result = rewrite_login(payload, self.session)
        assert result.new_payload is not None
        assert struct.unpack(">i", result.new_payload[:4])[0] == 924
        assert len(result.new_payload) == len(payload)

    def test_no_rewrite_when_matching(self):
        payload = struct.pack(">i", 924) + b"\x00" * 100
        result = rewrite_login(payload, self.session)
        assert result.new_payload is None


class TestV924ToV944Translator:
    def test_cancels_new_serverbound_packets(self):
        from endstone_endweave.protocol.v924_to_v944.translator import create_v924_to_v944

        translator = create_v924_to_v944()
        session = PlayerSession(
            address="1.2.3.4:1234", client_protocol=944, server_protocol=924
        )

        # ServerboundDataDrivenScreenClosed (343) - new in v944
        result = translator.translate_serverbound(343, b"\x00", session)
        assert result.cancel

    def test_passthrough_normal_packets(self):
        from endstone_endweave.protocol.v924_to_v944.translator import create_v924_to_v944

        translator = create_v924_to_v944()
        session = PlayerSession(
            address="1.2.3.4:1234", client_protocol=944, server_protocol=924
        )

        # Some normal gameplay packet
        result = translator.translate_serverbound(50, b"\x00\x01\x02", session)
        assert not result.cancel
        assert result.new_payload is None
