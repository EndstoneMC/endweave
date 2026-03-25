"""Tests for error scenarios: truncated payloads, structured error context."""

import struct
from unittest.mock import MagicMock

from endstone_endweave.connection import ConnectionManager
from endstone_endweave.debug import DebugHandler, packet_label
from endstone_endweave.exception import InformativeException
from endstone_endweave.pipeline import ProtocolPipeline
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.base import create_base_protocol
from endstone_endweave.protocol.manager import ProtocolManager


def _make_event(packet_id, payload, address="1.2.3.4:1234"):
    event = MagicMock()
    event.packet_id = packet_id
    event.payload = payload
    event.address = address
    return event


def _setup_pipeline(server_protocol=924):
    logger = MagicMock()
    connections = ConnectionManager(server_protocol=server_protocol, logger=logger)
    manager = ProtocolManager()
    manager.register_base(create_base_protocol(server_protocol))
    pipeline = ProtocolPipeline(manager, connections, logger)
    return pipeline, connections, manager, logger


class TestTruncatedPayload:
    def test_truncated_serverbound_cancels_packet(self):
        """A handler that reads beyond the payload should cancel, not crash."""
        pipeline, connections, manager, logger = _setup_pipeline()

        def greedy_handler(wrapper):
            from endstone_endweave.codec import INT_LE

            # Try to read 4 bytes from a 1-byte payload
            wrapper.passthrough(INT_LE)

        protocol = Protocol(server_protocol=924, client_protocol=944)
        protocol.register_serverbound(42, greedy_handler)
        manager.register(protocol)

        # Detect client version
        rns = _make_event(193, struct.pack(">i", 944))
        pipeline.on_packet_receive(rns)

        # Send truncated packet
        event = _make_event(42, b"\x01")
        pipeline.on_packet_receive(event)
        event.cancel.assert_called_once()
        # Error should have been logged
        logger.error.assert_called_once()

    def test_truncated_clientbound_cancels_packet(self):
        pipeline, connections, manager, logger = _setup_pipeline()

        def greedy_handler(wrapper):
            from endstone_endweave.codec import INT64_LE

            wrapper.passthrough(INT64_LE)

        protocol = Protocol(server_protocol=924, client_protocol=944)
        protocol.register_clientbound(42, greedy_handler)
        manager.register(protocol)

        rns = _make_event(193, struct.pack(">i", 944))
        pipeline.on_packet_receive(rns)

        # Trigger pipeline resolution via a second serverbound packet
        sb_event = _make_event(99, b"\x00")
        pipeline.on_packet_receive(sb_event)

        event = _make_event(42, b"\x01\x02")
        pipeline.on_packet_send(event)
        event.cancel.assert_called_once()
        logger.error.assert_called_once()


class TestInformativeExceptionContext:
    def test_set_chaining(self):
        cause = ValueError("bad data")
        err = InformativeException(cause).set("Direction", "SB").set("Packet", "FOO(1)")
        msg = err.message
        assert "Direction: SB" in msg
        assert "Packet: FOO(1)" in msg
        assert "bad data" in msg

    def test_format_includes_cause_type(self):
        cause = IndexError("out of bounds")
        err = InformativeException(cause)
        msg = err.message
        assert "IndexError" in msg
        assert "out of bounds" in msg

    def test_add_source(self):
        cause = RuntimeError("boom")
        err = InformativeException(cause).add_source(dict).add_source(list)
        msg = err.message
        assert "Source 0: dict" in msg
        assert "Source 1: list" in msg

    def test_comma_separated_format(self):
        cause = RuntimeError("x")
        err = InformativeException(cause).set("A", "1").set("B", "2")
        msg = err.message
        assert "A: 1, B: 2" in msg

    def test_error_log_contains_structured_context(self):
        """Verify pipeline error log includes protocol name and direction."""
        pipeline, connections, manager, logger = _setup_pipeline()

        def bad_handler(wrapper):
            raise RuntimeError("test explosion")

        protocol = Protocol(server_protocol=924, client_protocol=944, name="v924_to_v944")
        protocol.register_serverbound(42, bad_handler)
        manager.register(protocol)

        rns = _make_event(193, struct.pack(">i", 944))
        pipeline.on_packet_receive(rns)

        event = _make_event(42, b"\x00")
        pipeline.on_packet_receive(event)
        event.cancel.assert_called_once()

        error_msg = logger.error.call_args[0][0]
        assert "Direction: SERVERBOUND" in error_msg
        assert "Protocol: v924_to_v944" in error_msg
        assert "Address: 1.2.3.4:1234" in error_msg
        assert "Please report" in error_msg


class TestDebugHandler:
    def test_disabled_logs_nothing(self):
        logger = MagicMock()
        handler = DebugHandler(logger, enabled=False)
        handler.log(42, "test message")
        logger.debug.assert_not_called()

    def test_enabled_no_filter_logs_everything(self):
        logger = MagicMock()
        handler = DebugHandler(logger, enabled=True)
        handler.log(42, "msg1")
        handler.log(99, "msg2")
        assert logger.debug.call_count == 2

    def test_enabled_with_filter(self):
        logger = MagicMock()
        handler = DebugHandler(logger, enabled=True, packets=frozenset({42}))
        handler.log(42, "should log")
        handler.log(99, "should not log")
        assert logger.debug.call_count == 1
        assert "should log" in logger.debug.call_args[0][0]

    def test_log_packet_via_format(self):
        """Verify packet log format."""
        logger = MagicMock()
        handler = DebugHandler(logger, enabled=True)
        handler.log_packet("PRE ", "1.2.3.4:1234", "SERVERBOUND", "LOGIN", 11, 944, 256)
        msg = logger.debug.call_args[0][0]
        assert "PRE :" in msg
        assert "SERVERBOUND" in msg
        assert "LOGIN" in msg
        assert "START_GAME(11)" in msg
        assert "0x0B" in msg
        assert "[944]" in msg
        assert "256b" in msg

    def test_pre_post_flags(self):
        logger = MagicMock()
        handler = DebugHandler(logger, enabled=True, log_pre=True, log_post=False)
        assert handler.log_pre_packet_transform
        assert not handler.log_post_packet_transform

    def test_from_config(self):
        logger = MagicMock()
        config = {"debug": {"enabled": True, "packets": [11, 193]}}
        handler = DebugHandler.from_config(logger, config)
        assert handler.enabled
        assert handler.should_log(11)
        assert handler.should_log(193)
        assert not handler.should_log(42)
        assert handler.log_pre_packet_transform
        assert not handler.log_post_packet_transform

    def test_from_config_with_post_transform(self):
        logger = MagicMock()
        config = {"debug": {"enabled": True, "log_post_transform": True}}
        handler = DebugHandler.from_config(logger, config)
        assert handler.log_post_packet_transform

    def test_from_config_empty(self):
        logger = MagicMock()
        handler = DebugHandler.from_config(logger, {})
        assert not handler.enabled


class TestPacketLabel:
    def test_known_packet_id(self):
        """Verify hex format: NAME(id) (0xHH)."""
        label = packet_label(11)
        assert "START_GAME(11)" in label
        assert "(0x0B)" in label

    def test_single_digit_hex_padded(self):
        label = packet_label(5)
        assert "(0x05)" in label

    def test_two_digit_hex(self):
        label = packet_label(193)  # RequestNetworkSettings = 0xC1
        assert "(0xC1)" in label

    def test_unknown_packet_id(self):
        label = packet_label(9999)
        assert "9999" in label
        assert "(0x270F)" in label
