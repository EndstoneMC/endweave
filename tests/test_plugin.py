"""Version detection off the handshake, the blocked version gate, and what the plugin declares."""

from __future__ import annotations

from collections.abc import Callable
from pathlib import Path
from unittest.mock import MagicMock
from uuid import uuid4

import pytest
from endstone.command import Command

from endweave.config import EndweaveConfig
from endweave.connection import ConnectionManager
from endweave.plugin import EndweavePlugin
from endweave.protocol.version import get_protocol

SERVER_PROTOCOL = get_protocol(944)
ADDRESS = "127.0.0.1:19132"


class StubPlugin(EndweavePlugin):
    """The plugin as ``on_enable`` leaves it, with the server behind it stubbed out."""

    def __init__(self, configuration: EndweaveConfig, logger: MagicMock) -> None:
        super().__init__()
        self._stub_logger = logger
        self._configuration = configuration
        self._connection_manager = ConnectionManager(SERVER_PROTOCOL)

    @property
    def logger(self) -> MagicMock:
        return self._stub_logger


@pytest.fixture
def make_plugin(tmp_path: Path, mock_logger: MagicMock) -> Callable[..., StubPlugin]:
    def make(config_text: str = "") -> StubPlugin:
        config_file = tmp_path / "config.toml"
        config_file.write_text(config_text, encoding="utf-8")
        configuration = EndweaveConfig(config_file, mock_logger)
        configuration.reload()
        return StubPlugin(configuration, mock_logger)

    return make


def handshake(protocol: int, address: str = ADDRESS, packet_id: int = 193) -> MagicMock:
    event = MagicMock()
    event.packet_id = packet_id
    event.payload = protocol.to_bytes(4, "big", signed=True)
    event.address = address
    return event


def login(
    address: str = ADDRESS,
    name: str = "Steve",
    game_version: str = "1.26.10",
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


class TestVersionDetection:
    def test_reads_the_client_version_off_the_handshake(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()

        plugin.on_packet_receive(handshake(975))

        assert plugin._connection_manager.get_connection(ADDRESS).protocol_version == get_protocol(975)

    def test_reads_a_version_it_does_not_know(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()

        plugin.on_packet_receive(handshake(9999))

        connection = plugin._connection_manager.get_connection(ADDRESS)
        assert connection.protocol_version.version == 9999
        assert connection.protocol_version.known is False

    def test_ignores_any_other_packet(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()

        plugin.on_packet_receive(handshake(975, packet_id=1))

        assert not plugin._connection_manager.connections

    def test_ignores_a_handshake_too_short_to_hold_a_version(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()
        event = handshake(975)
        event.payload = b"\x00\x03"

        plugin.on_packet_receive(event)

        assert not plugin._connection_manager.connections

    def test_creates_no_connection_for_a_version_it_cannot_resolve(
        self, make_plugin: Callable[..., StubPlugin], monkeypatch: pytest.MonkeyPatch
    ) -> None:
        plugin = make_plugin()
        monkeypatch.setattr("endweave.plugin.get_protocol", MagicMock(side_effect=ValueError("bad version")))

        with pytest.raises(ValueError):
            plugin.on_packet_receive(handshake(975))

        assert not plugin._connection_manager.connections

    def test_drops_the_oldest_handshake_that_never_logged_in(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()

        for port in range(1025):
            plugin.on_packet_receive(handshake(975, address=f"127.0.0.1:{port}"))

        connections = plugin._connection_manager.connections
        assert len(connections) == 1024
        assert "127.0.0.1:0" not in connections
        assert "127.0.0.1:1024" in connections


class TestLogin:
    def test_tracks_the_connection_of_a_player_that_may_join(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()
        plugin.on_packet_receive(handshake(975))
        event = login()

        plugin.on_player_login(event)

        event.cancel.assert_not_called()
        connection = plugin._connection_manager.get_connection(ADDRESS)
        assert connection is not None
        assert connection.player is event.player
        assert connection.protocol_version == get_protocol(975)
        assert connection.server_protocol_version == SERVER_PROTOCOL

    def test_keeps_the_connection_the_handshake_opened(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()
        plugin.on_packet_receive(handshake(975))
        connection = plugin._connection_manager.get_connection(ADDRESS)

        plugin.on_player_login(login())

        assert plugin._connection_manager.get_connection(ADDRESS) is connection

    def test_tracks_nothing_for_a_player_that_never_shook_hands(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin()
        event = login()

        plugin.on_player_login(event)

        event.cancel.assert_not_called()
        assert plugin._connection_manager.get_connection(ADDRESS) is None

    def test_drops_the_connection_of_a_login_another_plugin_cancelled(
        self, make_plugin: Callable[..., StubPlugin]
    ) -> None:
        plugin = make_plugin()
        plugin.on_packet_receive(handshake(975))
        connection = plugin._connection_manager.get_connection(ADDRESS)

        plugin.on_player_login(login(cancelled=True))

        assert plugin._connection_manager.get_connection(ADDRESS) is None
        assert connection.player is None
        assert connection.active is False

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


class TestBlockedVersions:
    def test_refuses_a_blocked_protocol(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin("block-protocols = [975]\n")
        plugin.on_packet_receive(handshake(975))
        event = login()

        plugin.on_player_login(event)

        event.cancel.assert_called_once()
        assert event.kick_message == "You are using an unsupported Minecraft version!"
        assert plugin._connection_manager.get_connection(ADDRESS) is None

    def test_refuses_a_version_beyond_a_bound(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin('block-versions = ["<1.26.0"]\n')
        plugin.on_packet_receive(handshake(898))
        event = login()

        plugin.on_player_login(event)

        event.cancel.assert_called_once()

    def test_lets_a_version_the_config_allows_through(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin("block-protocols = [975]\n")
        plugin.on_packet_receive(handshake(944))
        event = login()

        plugin.on_player_login(event)

        event.cancel.assert_not_called()

    def test_refuses_a_blocked_client_whose_handshake_was_evicted(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin("block-protocols = [975]\n")
        plugin.on_packet_receive(handshake(975))
        for port in range(1025):
            plugin.on_packet_receive(handshake(944, address=f"127.0.0.1:{port}"))
        event = login(game_version="1.26.20")

        plugin.on_player_login(event)

        event.cancel.assert_called_once()

    def test_refuses_a_blocked_client_that_never_shook_hands(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin("block-protocols = [975]\n")
        event = login(game_version="1.26.20")

        plugin.on_player_login(event)

        event.cancel.assert_called_once()

    def test_reads_the_handshake_before_the_game_version(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin("block-protocols = [9999]\n")
        plugin.on_packet_receive(handshake(9999))
        event = login(game_version="1.26.10")

        plugin.on_player_login(event)

        event.cancel.assert_called_once()

    def test_refuses_an_unidentifiable_client_below_a_lower_bound(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin('block-versions = ["<1.26.0"]\n')
        event = login(game_version="1.99.0")

        plugin.on_player_login(event)

        event.cancel.assert_called_once()

    def test_admits_an_unidentifiable_client_when_nothing_is_blocked(
        self, make_plugin: Callable[..., StubPlugin]
    ) -> None:
        plugin = make_plugin()
        event = login(game_version="1.99.0")

        plugin.on_player_login(event)

        event.cancel.assert_not_called()

    def test_translates_the_colour_codes_of_the_kick_message(self, make_plugin: Callable[..., StubPlugin]) -> None:
        plugin = make_plugin('block-protocols = [975]\nblock-disconnect-msg = "&cGo away"\n')
        plugin.on_packet_receive(handshake(975))
        event = login()

        plugin.on_player_login(event)

        assert event.kick_message == "§cGo away"

    def test_says_nothing_about_a_refused_join_by_default(
        self, make_plugin: Callable[..., StubPlugin], mock_logger: MagicMock
    ) -> None:
        plugin = make_plugin("block-protocols = [975]\n")
        plugin.on_packet_receive(handshake(975))

        plugin.on_player_login(login())

        mock_logger.info.assert_not_called()

    def test_reports_a_refused_join_when_asked_to(
        self, make_plugin: Callable[..., StubPlugin], mock_logger: MagicMock
    ) -> None:
        plugin = make_plugin("block-protocols = [975]\n[logging]\nlog-blocked-joins = true\n")
        plugin.on_packet_receive(handshake(975))

        plugin.on_player_login(login())

        mock_logger.info.assert_called_once_with(f"Blocked join due to unsupported version from {ADDRESS} (1.26.20)")


class TestDeclaration:
    def test_gates_the_command_behind_a_permission_endstone_reads(self) -> None:
        command = Command("endweave", **EndweavePlugin.commands["endweave"])

        assert command.permissions == ["endweave.command"]

    def test_grants_the_command_permission_to_an_admin(self) -> None:
        children = EndweavePlugin.permissions["endweave.admin"]["children"]

        assert children["endweave.command"] is True

    def test_offers_a_usage_with_no_subcommand(self) -> None:
        assert "/endweave" in EndweavePlugin.commands["endweave"]["usages"]
