"""Tests for session management and packet routing pipeline."""

from __future__ import annotations

import struct
from unittest.mock import MagicMock

import pytest

from endstone_endweave.pipeline import TranslationPipeline
from endstone_endweave.player_state import PlayerSession, SessionManager
from endstone_endweave.protocol.base import PacketTransformation, ProtocolTranslator
from endstone_endweave.protocol.registry import TranslatorRegistry


class TestPlayerSession:
    def test_needs_translation_false_when_matching(self):
        s = PlayerSession(address="1.2.3.4:1234", client_protocol=924, server_protocol=924)
        assert not s.needs_translation

    def test_needs_translation_true_when_different(self):
        s = PlayerSession(address="1.2.3.4:1234", client_protocol=944, server_protocol=924)
        assert s.needs_translation

    def test_needs_translation_false_when_undetected(self):
        s = PlayerSession(address="1.2.3.4:1234")
        assert not s.needs_translation


class TestSessionManager:
    def test_get_or_create(self):
        mgr = SessionManager()
        s1 = mgr.get_or_create("1.2.3.4:1234")
        s2 = mgr.get_or_create("1.2.3.4:1234")
        assert s1 is s2

    def test_get_nonexistent(self):
        mgr = SessionManager()
        assert mgr.get("nope") is None

    def test_remove_by_address(self):
        mgr = SessionManager()
        mgr.get_or_create("1.2.3.4:1234")
        mgr.remove_by_address("1.2.3.4:1234")
        assert mgr.get("1.2.3.4:1234") is None

    def test_remove_by_player(self):
        mgr = SessionManager()
        mgr.get_or_create("1.2.3.4:1234")
        player = MagicMock()
        player.address = "1.2.3.4:1234"
        mgr.remove_by_player(player)
        assert mgr.get("1.2.3.4:1234") is None


class TestTranslatorRegistry:
    def test_register_and_get(self):
        reg = TranslatorRegistry()
        translator = ProtocolTranslator(server_protocol=924, client_protocol=944)
        reg.register(translator)
        assert reg.get(924, 944) is translator

    def test_get_missing(self):
        reg = TranslatorRegistry()
        assert reg.get(924, 999) is None


class TestTranslationPipeline:
    def _make_event(self, packet_id: int, payload: bytes, address: str = "1.2.3.4:1234"):
        event = MagicMock()
        event.packet_id = packet_id
        event.payload = payload
        event.address = address
        return event

    def _make_pipeline(self):
        sessions = SessionManager()
        registry = TranslatorRegistry()
        logger = MagicMock()
        pipeline = TranslationPipeline(registry, sessions, logger)
        return pipeline, sessions, registry

    def test_request_network_settings_creates_session(self):
        pipeline, sessions, _ = self._make_pipeline()
        payload = struct.pack(">i", 944)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)
        session = sessions.get("1.2.3.4:1234")
        assert session is not None
        assert session.client_protocol == 944

    def test_passthrough_when_no_translation_needed(self):
        pipeline, sessions, _ = self._make_pipeline()
        # Create session with matching protocol
        payload = struct.pack(">i", 924)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)
        # Subsequent packet should pass through
        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        event2.cancel.assert_not_called()

    def test_passthrough_when_no_translator(self):
        pipeline, sessions, _ = self._make_pipeline()
        # Create session with unknown protocol
        payload = struct.pack(">i", 999)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)
        # Next packet should pass through unmodified (no translator for 924->999)
        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        event2.cancel.assert_not_called()

    def test_translator_called_for_serverbound(self):
        pipeline, sessions, registry = self._make_pipeline()
        translator = MagicMock(spec=ProtocolTranslator)
        translator.server_protocol = 924
        translator.client_protocol = 944
        translator.translate_serverbound.return_value = PacketTransformation(
            new_payload=b"\xff"
        )
        registry.register(translator)

        # Setup session
        payload = struct.pack(">i", 944)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)

        # Send a packet that should be translated
        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        # Called twice: once for the RequestNetworkSettings (193), once for packet 42
        assert translator.translate_serverbound.call_count == 2
        translator.translate_serverbound.assert_called_with(
            42, b"\x00\x01", sessions.get("1.2.3.4:1234")
        )
        assert event2.payload == b"\xff"

    def test_send_passthrough_without_session(self):
        pipeline, _, _ = self._make_pipeline()
        event = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_send(event)
        event.cancel.assert_not_called()
