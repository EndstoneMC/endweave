"""Tests for connection management and packet routing pipeline."""

import struct
from unittest.mock import MagicMock

from endstone_endweave.codec import REMAINING_BYTES
from endstone_endweave.codec.wrapper import PacketWrapper
from endstone_endweave.connection import ConnectionManager, UserConnection
from endstone_endweave.pipeline import ProtocolPipeline
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.base import create_base_protocol
from endstone_endweave.protocol.manager import ProtocolManager


class TestUserConnection:
    def test_needs_translation_false_when_matching(self):
        c = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=924,
            server_protocol=924,
        )
        assert not c.needs_translation

    def test_needs_translation_true_when_different(self):
        c = UserConnection(
            address="1.2.3.4:1234",
            logger=MagicMock(),
            client_protocol=944,
            server_protocol=924,
        )
        assert c.needs_translation

    def test_needs_translation_false_when_undetected(self):
        c = UserConnection(address="1.2.3.4:1234", logger=MagicMock())
        assert not c.needs_translation


class TestConnectionManager:
    def test_get_or_create(self):
        mgr = ConnectionManager()
        c1 = mgr.get_or_create("1.2.3.4:1234")
        c2 = mgr.get_or_create("1.2.3.4:1234")
        assert c1 is c2

    def test_get_nonexistent(self):
        mgr = ConnectionManager()
        assert mgr.get("nope") is None

    def test_remove_by_address(self):
        mgr = ConnectionManager()
        mgr.get_or_create("1.2.3.4:1234")
        mgr.remove_by_address("1.2.3.4:1234")
        assert mgr.get("1.2.3.4:1234") is None

    def test_remove_by_player(self):
        mgr = ConnectionManager()
        mgr.get_or_create("1.2.3.4:1234")
        player = MagicMock()
        player.address = "1.2.3.4:1234"
        mgr.remove_by_player(player)
        assert mgr.get("1.2.3.4:1234") is None


class TestProtocolManager:
    def test_register_and_get(self):
        mgr = ProtocolManager()
        protocol = Protocol(server_protocol=924, client_protocol=944)
        mgr.register(protocol)
        assert mgr.get(924, 944) is protocol

    def test_get_missing(self):
        mgr = ProtocolManager()
        assert mgr.get(924, 999) is None


class TestProtocolPipeline:
    def _make_event(self, packet_id: int, payload: bytes, address: str = "1.2.3.4:1234"):
        event = MagicMock()
        event.packet_id = packet_id
        event.payload = payload
        event.address = address
        return event

    def _make_pipeline(self, server_protocol: int = 924):
        logger = MagicMock()
        connections = ConnectionManager(server_protocol=server_protocol, logger=logger)
        manager = ProtocolManager()
        manager.register_base(create_base_protocol(server_protocol))
        pipeline = ProtocolPipeline(manager, connections, logger)
        return pipeline, connections, manager

    def test_request_network_settings_creates_session(self):
        pipeline, connections, _ = self._make_pipeline()
        payload = struct.pack(">i", 944)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)
        connection = connections.get("1.2.3.4:1234")
        assert connection is not None
        assert connection.client_protocol == 944

    def test_passthrough_when_no_translation_needed(self):
        pipeline, connections, _ = self._make_pipeline()
        payload = struct.pack(">i", 924)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)
        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        event2.cancel.assert_not_called()

    def test_passthrough_when_no_protocol_chain(self):
        pipeline, connections, _ = self._make_pipeline()
        payload = struct.pack(">i", 999)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)
        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        event2.cancel.assert_not_called()

    def test_protocol_called_for_serverbound(self):
        pipeline, connections, manager = self._make_pipeline()

        def rewrite_handler(wrapper: PacketWrapper) -> None:
            wrapper.read(REMAINING_BYTES)
            wrapper.write(REMAINING_BYTES, b"\xff")

        protocol = Protocol(server_protocol=924, client_protocol=944)
        protocol.register_serverbound(42, rewrite_handler)
        manager.register(protocol)

        payload = struct.pack(">i", 944)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)

        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        assert event2.payload == b"\xff"

    def test_send_passthrough_without_session(self):
        pipeline, _, _ = self._make_pipeline()
        event = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_send(event)
        event.cancel.assert_not_called()

    def test_serverbound_exception_cancels_packet(self):
        """A handler that raises should cancel the packet, not pass it through."""
        pipeline, connections, manager = self._make_pipeline()

        def bad_handler(wrapper: PacketWrapper) -> None:
            raise ValueError("boom")

        protocol = Protocol(server_protocol=924, client_protocol=944)
        protocol.register_serverbound(42, bad_handler)
        manager.register(protocol)

        # Detect client version first
        payload = struct.pack(">i", 944)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)

        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_receive(event2)
        event2.cancel.assert_called_once()

    def test_clientbound_exception_cancels_packet(self):
        """A handler that raises on send should cancel the packet."""
        pipeline, connections, manager = self._make_pipeline()

        def bad_handler(wrapper: PacketWrapper) -> None:
            raise ValueError("boom")

        protocol = Protocol(server_protocol=924, client_protocol=944)
        protocol.register_clientbound(42, bad_handler)
        manager.register(protocol)

        # Detect client version first
        payload = struct.pack(">i", 944)
        event = self._make_event(193, payload)
        pipeline.on_packet_receive(event)

        # Trigger pipeline resolution via a second serverbound packet
        event_sb = self._make_event(99, b"\x00")
        pipeline.on_packet_receive(event_sb)

        event2 = self._make_event(42, b"\x00\x01")
        pipeline.on_packet_send(event2)
        event2.cancel.assert_called_once()


class TestGetVersionByName:
    def test_patch_version_maps_to_base(self):
        from endstone_endweave.protocol.versions import get_version_by_name, v1_26_0

        assert get_version_by_name("1.26.2") is v1_26_0

    def test_exact_version_match(self):
        from endstone_endweave.protocol.versions import get_version_by_name, v1_26_10

        assert get_version_by_name("1.26.10") is v1_26_10

    def test_unknown_version_returns_none(self):
        from endstone_endweave.protocol.versions import get_version_by_name

        assert get_version_by_name("1.99.0") is None
