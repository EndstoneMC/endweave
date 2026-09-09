"""DebugHandler defaults, the packet type filter, and error logging."""

from __future__ import annotations

from dataclasses import dataclass
from unittest.mock import MagicMock

import pytest

from endweave.debug import DebugHandler, Direction, PacketType

START_GAME = PacketType(11, "START_GAME", Direction.CLIENTBOUND)
START_GAME_SERVERBOUND = PacketType(11, "START_GAME", Direction.SERVERBOUND)
REQUEST_NETWORK_SETTINGS = PacketType(193, "REQUEST_NETWORK_SETTINGS", Direction.SERVERBOUND)


@dataclass(frozen=True)
class StubPacket:
    """Minimal LoggablePacket: an ID, plus a type once one has been resolved."""

    packet_id: int
    packet_type: PacketType | None = None


@pytest.fixture
def handler(mock_logger: MagicMock) -> DebugHandler:
    return DebugHandler(mock_logger)


@pytest.fixture
def failure() -> ValueError:
    """A raised-and-caught exception, so it carries a traceback."""
    try:
        raise ValueError("truncated varint")
    except ValueError as exc:
        return exc


class TestDefaults:
    def test_starts_disabled_logging_only_pre_transform(self, handler: DebugHandler) -> None:
        assert not handler.enabled
        assert handler.log_pre_packet_transform
        assert not handler.log_post_packet_transform

    def test_set_log_packet_transform_sets_both_phases(self, handler: DebugHandler) -> None:
        handler.set_log_packet_transform(True)
        assert handler.log_pre_packet_transform
        assert handler.log_post_packet_transform

        handler.set_log_packet_transform(False)
        assert not handler.log_pre_packet_transform
        assert not handler.log_post_packet_transform


class TestPacketFilter:
    def test_empty_filter_logs_everything(self, handler: DebugHandler) -> None:
        assert handler.should_log(StubPacket(11, START_GAME), Direction.CLIENTBOUND)
        assert handler.should_log(StubPacket(9999), Direction.SERVERBOUND)

    def test_filtering_is_independent_of_enabled(self, handler: DebugHandler) -> None:
        handler.add_packet_type_to_log(START_GAME)
        assert not handler.enabled
        assert handler.should_log(StubPacket(11, START_GAME), Direction.CLIENTBOUND)
        assert not handler.should_log(StubPacket(193, REQUEST_NETWORK_SETTINGS), Direction.SERVERBOUND)

    def test_name_matches_in_either_direction(self, handler: DebugHandler) -> None:
        handler.add_packet_type_name_to_log("START_GAME")
        assert handler.should_log(StubPacket(11, START_GAME), Direction.CLIENTBOUND)
        assert handler.should_log(StubPacket(11, START_GAME_SERVERBOUND), Direction.SERVERBOUND)
        assert not handler.should_log(StubPacket(193, REQUEST_NETWORK_SETTINGS), Direction.SERVERBOUND)

    def test_type_matches_only_its_own_direction(self, handler: DebugHandler) -> None:
        handler.add_packet_type_to_log(START_GAME_SERVERBOUND)
        assert handler.should_log(StubPacket(11, START_GAME_SERVERBOUND), Direction.SERVERBOUND)
        assert not handler.should_log(StubPacket(11, START_GAME), Direction.CLIENTBOUND)

    def test_unresolved_type_falls_back_to_packet_id(self, handler: DebugHandler) -> None:
        handler.add_packet_type_to_log(REQUEST_NETWORK_SETTINGS)
        assert handler.should_log(StubPacket(193), Direction.SERVERBOUND)
        assert not handler.should_log(StubPacket(193), Direction.CLIENTBOUND)
        assert not handler.should_log(StubPacket(11), Direction.SERVERBOUND)

    def test_name_filter_alone_never_matches_an_unresolved_type(self, handler: DebugHandler) -> None:
        handler.add_packet_type_name_to_log("START_GAME")
        assert not handler.should_log(StubPacket(11), Direction.CLIENTBOUND)


class TestFilterRemoval:
    def test_remove_name_reports_whether_it_was_filtered(self, handler: DebugHandler) -> None:
        handler.add_packet_type_name_to_log("START_GAME")
        assert handler.remove_packet_type_name_to_log("START_GAME")
        assert not handler.remove_packet_type_name_to_log("START_GAME")

    def test_remove_type_reports_whether_it_was_filtered(self, handler: DebugHandler) -> None:
        handler.add_packet_type_to_log(START_GAME)
        assert not handler.remove_packet_type_to_log(START_GAME_SERVERBOUND)
        assert handler.remove_packet_type_to_log(START_GAME)
        assert not handler.remove_packet_type_to_log(START_GAME)

    def test_emptying_the_filter_logs_everything_again(self, handler: DebugHandler) -> None:
        handler.add_packet_type_to_log(START_GAME)
        assert not handler.should_log(StubPacket(193, REQUEST_NETWORK_SETTINGS), Direction.SERVERBOUND)

        handler.remove_packet_type_to_log(START_GAME)
        assert handler.should_log(StubPacket(193, REQUEST_NETWORK_SETTINGS), Direction.SERVERBOUND)

    def test_clear_resets_names_and_types(self, handler: DebugHandler) -> None:
        handler.add_packet_type_name_to_log("START_GAME")
        handler.add_packet_type_to_log(REQUEST_NETWORK_SETTINGS)
        handler.clear_packet_types_to_log()
        assert handler.should_log(StubPacket(42), Direction.CLIENTBOUND)


class TestEnableAndLogTypes:
    def test_enables_debug_and_filters_to_the_given_types(self, handler: DebugHandler) -> None:
        handler.enable_and_log_types(START_GAME, REQUEST_NETWORK_SETTINGS)
        assert handler.enabled
        assert handler.should_log(StubPacket(11, START_GAME), Direction.CLIENTBOUND)
        assert handler.should_log(StubPacket(193, REQUEST_NETWORK_SETTINGS), Direction.SERVERBOUND)
        text = PacketType(42, "TEXT", Direction.CLIENTBOUND)
        assert not handler.should_log(StubPacket(42, text), Direction.CLIENTBOUND)


class TestErrorLogging:
    def test_logs_while_conversion_warnings_are_on(self, mock_logger: MagicMock, failure: ValueError) -> None:
        DebugHandler(mock_logger).error("Failed to translate START_GAME", failure)
        mock_logger.error.assert_called_once()

    def test_logs_when_debug_is_on_though_warnings_are_muted(self, mock_logger: MagicMock, failure: ValueError) -> None:
        handler = DebugHandler(mock_logger, enabled=True, log_conversion_warnings=False)
        handler.error("Failed to translate START_GAME", failure)
        mock_logger.error.assert_called_once()

    def test_stays_quiet_when_muted_and_disabled(self, mock_logger: MagicMock, failure: ValueError) -> None:
        handler = DebugHandler(mock_logger, log_conversion_warnings=False)
        handler.error("Failed to translate START_GAME", failure)
        mock_logger.error.assert_not_called()

    def test_message_carries_the_traceback(self, mock_logger: MagicMock, failure: ValueError) -> None:
        DebugHandler(mock_logger).error("Failed to translate START_GAME", failure)
        message = mock_logger.error.call_args[0][0]
        assert "Failed to translate START_GAME" in message
        assert "ValueError: truncated varint" in message
        assert "Traceback (most recent call last)" in message
