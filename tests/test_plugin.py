"""The packet hooks and their priorities, dropping a connection on quit, and what the plugin declares."""

from __future__ import annotations

from collections.abc import Callable
from pathlib import Path
from unittest.mock import MagicMock
from uuid import uuid4

import pytest
from endstone.command import Command
from endstone.event import EventPriority

from endweave.config import EndweaveConfig
from endweave.connection import ConnectionManager
from endweave.debug import DebugHandler
from endweave.plugin import EndweavePlugin
from endweave.protocol.base import BaseProtocol
from endweave.protocol.version import ProtocolVersion, get_protocol

SERVER_PROTOCOL = get_protocol(944)
ADDRESS = "127.0.0.1:19132"

# The pair the engine carries, so a connection between them has translators.
CARRIED_SERVER = get_protocol(2168)
CARRIED_CLIENT = 2192

CONTAINER_CLOSE = 47
INVENTORY_TRANSACTION = 30
SET_PLAYER_FURNACE_OPTIONS = 351  # 2168 has no such packet, so a 2192 client loses it
TEXT = 9  # unchanged between the two, so neither table names it

EMPTY_TRANSACTION = b"\x00\x00\x00\x00"
EMPTY_CONTAINER_CLOSE = b"\x00\x00\x00"


class StubPlugin(EndweavePlugin):
    """The plugin as ``on_enable`` leaves it, with the server behind it stubbed out."""

    def __init__(
        self,
        configuration: EndweaveConfig,
        logger: MagicMock,
        server_protocol: ProtocolVersion = SERVER_PROTOCOL,
    ) -> None:
        super().__init__()
        self._stub_logger = logger
        self._stub_server = MagicMock()
        self._configuration = configuration
        self._connection_manager = ConnectionManager(server_protocol)
        self._base_protocol = BaseProtocol(self._connection_manager, configuration, logger)
        self._debug_handler = DebugHandler(logger)

    @property
    def logger(self) -> MagicMock:
        return self._stub_logger

    @property
    def server(self) -> MagicMock:
        return self._stub_server


@pytest.fixture
def make_plugin(tmp_path: Path, mock_logger: MagicMock) -> Callable[..., StubPlugin]:
    def make(config_text: str = "", server_protocol: ProtocolVersion = SERVER_PROTOCOL) -> StubPlugin:
        config_file = tmp_path / "config.toml"
        config_file.write_text(config_text, encoding="utf-8")
        configuration = EndweaveConfig(config_file, mock_logger)
        configuration.reload()
        return StubPlugin(configuration, mock_logger, server_protocol)

    return make


def handshake(protocol: int, address: str = ADDRESS, packet_id: int = 193) -> MagicMock:
    event = MagicMock()
    event.packet_id = packet_id
    event.payload = protocol.to_bytes(4, "big", signed=True)
    event.address = address
    return event


def packet(packet_id: int, payload: bytes, address: str = ADDRESS) -> MagicMock:
    event = MagicMock()
    event.packet_id = packet_id
    event.payload = payload
    event.address = address
    return event


def login(
    address: str = ADDRESS,
    name: str = "Steve",
    game_version: str = "26.10",
    *,
    cancelled: bool = False,
) -> MagicMock:
    event = MagicMock()
    event.is_cancelled = cancelled
    event.player.address = address
    event.player.name = name
    event.player.game_version = game_version
    event.player.unique_id = uuid4()
    return event


class TestTranslation:
    """A 2192 client against a 2168 server, the one pair the engine carries."""

    @pytest.fixture
    def plugin(self, make_plugin: Callable[..., StubPlugin]) -> StubPlugin:
        plugin = make_plugin(server_protocol=CARRIED_SERVER)
        plugin.on_packet_receive(handshake(CARRIED_CLIENT))
        return plugin

    def test_carries_a_received_packet_towards_the_server(self, plugin: StubPlugin) -> None:
        event = packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION)

        plugin.on_packet_receive(event)

        event.cancel.assert_not_called()
        assert event.payload != EMPTY_TRANSACTION

    def test_carries_a_sent_packet_towards_the_client(self, plugin: StubPlugin) -> None:
        connection = plugin._connection_manager.get_connection(ADDRESS)
        expected = connection.clientbound.translate(CONTAINER_CLOSE, EMPTY_CONTAINER_CLOSE)
        event = packet(CONTAINER_CLOSE, EMPTY_CONTAINER_CLOSE)

        plugin.on_packet_send(event)

        event.cancel.assert_not_called()
        assert event.payload == expected

    def test_leaves_a_packet_both_versions_read_alike_untouched(self, plugin: StubPlugin) -> None:
        event = packet(TEXT, b"\x07")

        plugin.on_packet_receive(event)

        event.cancel.assert_not_called()
        assert event.payload == b"\x07"

    def test_drops_a_packet_the_server_has_no_room_for(self, plugin: StubPlugin) -> None:
        event = packet(SET_PLAYER_FURNACE_OPTIONS, b"\x00")

        plugin.on_packet_receive(event)

        event.cancel.assert_called_once()

    def test_kicks_a_peer_whose_packet_will_not_translate(self, plugin: StubPlugin, mock_logger: MagicMock) -> None:
        connection = plugin._connection_manager.get_connection(ADDRESS)
        connection.player = MagicMock()
        event = packet(INVENTORY_TRANSACTION, b"")

        plugin.on_packet_receive(event)

        event.cancel.assert_called_once()
        connection.player.kick.assert_called_once()
        mock_logger.error.assert_called_once()

    def test_ignores_a_peer_that_never_shook_hands(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin(server_protocol=CARRIED_SERVER)
        event = packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION)

        plugin.on_packet_receive(event)

        event.cancel.assert_not_called()
        assert event.payload == EMPTY_TRANSACTION

    def test_leaves_a_client_on_the_server_version_alone(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin(server_protocol=CARRIED_SERVER)
        plugin.on_packet_receive(handshake(CARRIED_SERVER.version))
        event = packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION)

        plugin.on_packet_receive(event)

        event.cancel.assert_not_called()
        assert event.payload == EMPTY_TRANSACTION

    def test_leaves_a_client_the_engine_does_not_carry_alone(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin(server_protocol=CARRIED_SERVER)
        plugin.on_packet_receive(handshake(975))
        event = packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION)

        plugin.on_packet_receive(event)

        event.cancel.assert_not_called()
        assert event.payload == EMPTY_TRANSACTION

    def test_says_nothing_about_a_carried_packet_until_debugging_is_on(
        self, plugin: StubPlugin, mock_logger: MagicMock
    ) -> None:
        plugin.on_packet_receive(packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION))

        mock_logger.info.assert_not_called()

    def test_names_a_carried_packet_when_debugging_is_on(self, plugin: StubPlugin, mock_logger: MagicMock) -> None:
        plugin._debug_handler.enabled = True

        plugin.on_packet_receive(packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION))

        assert "INVENTORYTRANSACTION" in mock_logger.info.call_args.args[0]

    def test_says_nothing_about_a_packet_the_filter_excludes(self, plugin: StubPlugin, mock_logger: MagicMock) -> None:
        plugin._debug_handler.enabled = True
        plugin._debug_handler.add_packet_type_name_to_log("CONTAINERCLOSE")

        plugin.on_packet_receive(packet(INVENTORY_TRANSACTION, EMPTY_TRANSACTION))

        mock_logger.info.assert_not_called()


class TestPriority:
    def test_translates_a_received_packet_before_other_plugins_read_it(self) -> None:
        assert EndweavePlugin.on_packet_receive._priority == EventPriority.LOWEST

    def test_translates_a_sent_packet_once_other_plugins_have_read_it(self) -> None:
        assert EndweavePlugin.on_packet_send._priority == EventPriority.HIGHEST

    def test_leaves_a_send_another_plugin_cancelled_alone(self) -> None:
        assert EndweavePlugin.on_packet_send._ignore_cancelled is True


class TestQuit:
    def test_drops_the_connection_when_the_player_quits(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()
        plugin.on_packet_receive(handshake(975))
        event = login()
        plugin.on_player_login(event)
        connection = plugin._connection_manager.get_connection(ADDRESS)

        quit_event = MagicMock()
        quit_event.player = event.player
        plugin.on_player_quit(quit_event)

        assert plugin._connection_manager.get_connection(ADDRESS) is None
        assert connection.active is False

    def test_keeps_the_connection_of_a_player_still_on_the_same_address(
        self, make_plugin: Callable[..., StubPlugin]
    ) -> None:
        plugin = make_plugin()
        plugin.on_packet_receive(handshake(975))
        leaving = login(name="PlayerTwo")
        plugin.on_player_login(leaving)
        staying = login(name="PlayerOne")
        plugin.on_player_login(staying)
        connection = plugin._connection_manager.get_connection(ADDRESS)

        quit_event = MagicMock()
        quit_event.player = leaving.player
        plugin.on_player_quit(quit_event)

        assert plugin._connection_manager.get_connection(ADDRESS) is connection
        assert connection.player is staying.player
        assert connection.active is True


class TestReload:
    def test_kicks_every_online_player(self, make_plugin: Callable[..., StubPlugin], mock_logger: MagicMock) -> None:
        plugin = make_plugin('reload-disconnect-msg = "&cBack in a moment"\n')
        players = [MagicMock(), MagicMock()]
        plugin.server.online_players = players

        plugin.on_reload()

        for player in players:
            player.kick.assert_called_once_with("§cBack in a moment")
        mock_logger.error.assert_called_once()


class TestDeclaration:
    def test_gates_the_command_behind_a_permission_endstone_reads(self) -> None:
        command = Command("endweave", **EndweavePlugin.commands["endweave"])

        assert command.permissions == ["endweave.command"]

    def test_grants_the_command_permission_to_an_admin(self) -> None:
        children = EndweavePlugin.permissions["endweave.admin"]["children"]

        assert children["endweave.command"] is True

    def test_offers_a_usage_with_no_subcommand(self) -> None:
        assert "/endweave" in EndweavePlugin.commands["endweave"]["usages"]
