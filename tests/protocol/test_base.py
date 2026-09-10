"""The base protocol: reading the handshake, the pipeline it installs, the version BDS reads, and the login gate."""

from __future__ import annotations

from collections.abc import Callable
from pathlib import Path
from unittest.mock import MagicMock
from uuid import uuid4

import pytest

from endweave.config import EndweaveConfig
from endweave.connection import ConnectionManager
from endweave.protocol.base import BaseProtocol
from endweave.protocol.version import ProtocolVersion, get_protocol

SERVER_PROTOCOL = get_protocol(944)
ADDRESS = "127.0.0.1:19132"
NETHERNET_ADDRESS = ":0"

# The pair the engine carries, plus the version that is wire-identical to the older one.
CARRIED_SERVER = get_protocol(2168)
CARRIED_CLIENT = get_protocol(2192)
CARRIED_ALIAS = get_protocol(2169)

LOGIN = 1
CONNECTION_REQUEST = b"\x02{}"


@pytest.fixture
def make_protocol(tmp_path: Path, mock_logger: MagicMock) -> Callable[..., BaseProtocol]:
    def make(config_text: str = "", server_protocol: ProtocolVersion = SERVER_PROTOCOL) -> BaseProtocol:
        config_file = tmp_path / "config.toml"
        config_file.write_text(config_text, encoding="utf-8")
        configuration = EndweaveConfig(config_file, mock_logger)
        configuration.reload()
        return BaseProtocol(ConnectionManager(server_protocol), configuration, mock_logger)

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


class TestVersionDetection:
    def test_reads_the_client_version_off_the_handshake(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()

        protocol.transform_serverbound(handshake(975))

        assert protocol._connection_manager.get_connection(ADDRESS).protocol_version == get_protocol(975)

    def test_reads_a_version_it_does_not_know(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()

        protocol.transform_serverbound(handshake(9999))

        connection = protocol._connection_manager.get_connection(ADDRESS)
        assert connection.protocol_version.version == 9999
        assert connection.protocol_version.known is False

    def test_ignores_any_other_packet(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()

        protocol.transform_serverbound(handshake(975, packet_id=1))

        assert not protocol._connection_manager.connections

    def test_ignores_a_handshake_too_short_to_hold_a_version(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()
        event = handshake(975)
        event.payload = b"\x00\x03"

        protocol.transform_serverbound(event)

        assert not protocol._connection_manager.connections

    def test_creates_no_connection_for_a_version_it_cannot_resolve(
        self, make_protocol: Callable[..., BaseProtocol], monkeypatch: pytest.MonkeyPatch
    ) -> None:
        protocol = make_protocol()
        monkeypatch.setattr("endweave.protocol.base.get_protocol", MagicMock(side_effect=ValueError("bad version")))

        with pytest.raises(ValueError):
            protocol.transform_serverbound(handshake(975))

        assert not protocol._connection_manager.connections

    def test_drops_the_oldest_handshake_that_never_logged_in(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()

        for port in range(1025):
            protocol.transform_serverbound(handshake(975, address=f"127.0.0.1:{port}"))

        connections = protocol._connection_manager.connections
        assert len(connections) == 1024
        assert "127.0.0.1:0" not in connections
        assert "127.0.0.1:1024" in connections


class TestPipeline:
    def test_resolves_a_translator_for_each_direction(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)

        connection = protocol.transform_serverbound(handshake(CARRIED_CLIENT.version))

        assert connection.serverbound is not None
        assert (connection.serverbound.from_version, connection.serverbound.to_version) == (2192, 2168)
        assert connection.clientbound is not None
        assert (connection.clientbound.from_version, connection.clientbound.to_version) == (2168, 2192)

    def test_carries_nothing_when_both_ends_agree(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)

        connection = protocol.transform_serverbound(handshake(CARRIED_SERVER.version))

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_reads_a_wire_identical_client_as_the_version_it_speaks(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol(server_protocol=CARRIED_CLIENT)

        connection = protocol.transform_serverbound(handshake(CARRIED_ALIAS.version))

        assert connection.serverbound is not None
        assert connection.serverbound.from_version == 2168

    def test_leaves_a_wire_identical_client_alone_on_the_version_it_speaks(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)

        connection = protocol.transform_serverbound(handshake(CARRIED_ALIAS.version))

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_carries_nothing_for_a_client_the_engine_does_not_know(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)

        connection = protocol.transform_serverbound(handshake(975))

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_carries_nothing_for_a_server_the_engine_does_not_know(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol()

        connection = protocol.transform_serverbound(handshake(CARRIED_CLIENT.version))

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_re_resolves_when_the_client_version_changes(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)
        protocol.transform_serverbound(handshake(CARRIED_CLIENT.version))

        connection = protocol.transform_serverbound(handshake(975))

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_shares_translators_between_connections_on_the_same_versions(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)

        first = protocol.transform_serverbound(handshake(CARRIED_CLIENT.version, address="127.0.0.1:1"))
        second = protocol.transform_serverbound(handshake(CARRIED_CLIENT.version, address="127.0.0.1:2"))

        assert first is not second
        assert first.serverbound is second.serverbound
        assert first.clientbound is second.clientbound


class TestDeclaredVersion:
    """A 2192 client against a 2169 server, which BDS refuses unless it declares 2169."""

    @pytest.fixture
    def protocol(self, make_protocol: Callable[..., BaseProtocol]) -> BaseProtocol:
        return make_protocol(server_protocol=CARRIED_ALIAS)

    def test_writes_the_server_version_into_the_handshake(self, protocol: BaseProtocol) -> None:
        event = handshake(2192)

        protocol.transform_serverbound(event)

        assert event.payload == (2169).to_bytes(4, "big", signed=True)

    def test_writes_the_server_version_into_the_login(self, protocol: BaseProtocol) -> None:
        protocol.transform_serverbound(handshake(2192))
        event = packet(LOGIN, (2192).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST)

        protocol.transform_serverbound(event)

        assert event.payload == (2169).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST

    def test_leaves_a_client_on_the_server_version_untouched(self, protocol: BaseProtocol) -> None:
        event = handshake(2169)
        declared = event.payload

        protocol.transform_serverbound(event)

        assert event.payload is declared

    def test_leaves_a_client_on_the_wire_identical_version_to_endstone(self, protocol: BaseProtocol) -> None:
        handshake_event = handshake(2168)
        login_event = packet(LOGIN, (2168).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST)

        connection = protocol.transform_serverbound(handshake_event)
        protocol.transform_serverbound(login_event)

        assert connection.serverbound is None
        assert connection.clientbound is None
        assert handshake_event.payload == (2168).to_bytes(4, "big", signed=True)
        assert login_event.payload == (2168).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST

    def test_leaves_a_client_the_engine_does_not_carry_alone(self, protocol: BaseProtocol) -> None:
        handshake_event = handshake(975)
        login_event = packet(LOGIN, (975).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST)

        protocol.transform_serverbound(handshake_event)
        protocol.transform_serverbound(login_event)

        assert handshake_event.payload == (975).to_bytes(4, "big", signed=True)
        assert login_event.payload == (975).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST

    def test_leaves_a_handshake_too_short_to_hold_a_version_alone(self, protocol: BaseProtocol) -> None:
        event = packet(193, b"\x00\x03")

        connection = protocol.transform_serverbound(event)

        assert connection is None
        assert event.payload == b"\x00\x03"

    def test_leaves_a_login_too_short_to_hold_a_version_alone(self, protocol: BaseProtocol) -> None:
        protocol.transform_serverbound(handshake(2192))
        event = packet(LOGIN, b"\x00\x03")

        protocol.transform_serverbound(event)

        assert event.payload == b"\x00\x03"

    def test_leaves_the_login_of_a_peer_that_never_shook_hands_alone(self, protocol: BaseProtocol) -> None:
        event = packet(LOGIN, (2192).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST)

        protocol.transform_serverbound(event)

        assert event.payload == (2192).to_bytes(4, "big", signed=True) + CONNECTION_REQUEST


class TestNetherNet:
    """Endstone prints every NetherNet peer as the same address, so none of them can be told apart."""

    def test_tracks_no_nethernet_peer(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)
        event = handshake(2192, address=NETHERNET_ADDRESS)

        connection = protocol.transform_serverbound(event)

        assert connection is None
        assert not protocol._connection_manager.connections
        assert event.payload == (2192).to_bytes(4, "big", signed=True)

    def test_still_tracks_an_ipv6_peer(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol(server_protocol=CARRIED_SERVER)
        event = handshake(2192, address="::1:19132")

        connection = protocol.transform_serverbound(event)

        assert connection is not None
        assert connection.serverbound is not None
        assert event.payload == (2168).to_bytes(4, "big", signed=True)

    def test_lets_no_nethernet_login_take_over_another(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()
        protocol.transform_serverbound(handshake(975, address=NETHERNET_ADDRESS))
        protocol.on_login(login(address=NETHERNET_ADDRESS, name="PlayerOne"))
        protocol.transform_serverbound(handshake(975, address=NETHERNET_ADDRESS))

        protocol.on_login(login(address=NETHERNET_ADDRESS, name="PlayerTwo"))

        assert protocol._connection_manager.get_connection(NETHERNET_ADDRESS) is None

    def test_still_refuses_a_blocked_nethernet_client(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol("block-protocols = [975]\n")
        protocol.transform_serverbound(handshake(975, address=NETHERNET_ADDRESS))
        event = login(address=NETHERNET_ADDRESS, game_version="26.20")

        protocol.on_login(event)

        event.cancel.assert_called_once()


class TestLogin:
    def test_tracks_the_connection_of_a_player_that_may_join(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()
        protocol.transform_serverbound(handshake(975))
        event = login()

        protocol.on_login(event)

        event.cancel.assert_not_called()
        connection = protocol._connection_manager.get_connection(ADDRESS)
        assert connection is not None
        assert connection.player is event.player
        assert connection.protocol_version == get_protocol(975)
        assert connection.server_protocol_version == SERVER_PROTOCOL

    def test_keeps_the_connection_the_handshake_opened(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol()
        protocol.transform_serverbound(handshake(975))
        connection = protocol._connection_manager.get_connection(ADDRESS)

        protocol.on_login(login())

        assert protocol._connection_manager.get_connection(ADDRESS) is connection

    def test_tracks_nothing_for_a_player_that_never_shook_hands(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol()
        event = login()

        protocol.on_login(event)

        event.cancel.assert_not_called()
        assert protocol._connection_manager.get_connection(ADDRESS) is None

    def test_drops_the_connection_of_a_login_another_plugin_cancelled(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol()
        protocol.transform_serverbound(handshake(975))
        connection = protocol._connection_manager.get_connection(ADDRESS)

        protocol.on_login(login(cancelled=True))

        assert protocol._connection_manager.get_connection(ADDRESS) is None
        assert connection.player is None
        assert connection.active is False


class TestBlockedVersions:
    def test_refuses_a_blocked_protocol(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol("block-protocols = [975]\n")
        protocol.transform_serverbound(handshake(975))
        event = login()

        protocol.on_login(event)

        event.cancel.assert_called_once()
        assert event.kick_message == "You are using an unsupported Minecraft version!"
        assert protocol._connection_manager.get_connection(ADDRESS) is None

    def test_refuses_a_version_beyond_a_bound(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol('block-versions = ["<26.0"]\n')
        protocol.transform_serverbound(handshake(898))
        event = login()

        protocol.on_login(event)

        event.cancel.assert_called_once()

    def test_lets_a_version_the_config_allows_through(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol("block-protocols = [975]\n")
        protocol.transform_serverbound(handshake(944))
        event = login()

        protocol.on_login(event)

        event.cancel.assert_not_called()

    def test_refuses_a_blocked_client_whose_handshake_was_evicted(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol("block-protocols = [975]\n")
        protocol.transform_serverbound(handshake(975))
        for port in range(1025):
            protocol.transform_serverbound(handshake(944, address=f"127.0.0.1:{port}"))
        event = login(game_version="26.20")

        protocol.on_login(event)

        event.cancel.assert_called_once()

    def test_refuses_a_blocked_client_that_never_shook_hands(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol("block-protocols = [975]\n")
        event = login(game_version="1.26.20")

        protocol.on_login(event)

        event.cancel.assert_called_once()

    def test_reads_the_handshake_before_the_game_version(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol("block-protocols = [9999]\n")
        protocol.transform_serverbound(handshake(9999))
        event = login(game_version="26.10")

        protocol.on_login(event)

        event.cancel.assert_called_once()

    def test_refuses_an_unidentifiable_client_below_a_lower_bound(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol('block-versions = ["<26.0"]\n')
        event = login(game_version="1.99.0")

        protocol.on_login(event)

        event.cancel.assert_called_once()

    def test_admits_an_unidentifiable_client_when_nothing_is_blocked(
        self, make_protocol: Callable[..., BaseProtocol]
    ) -> None:
        protocol = make_protocol()
        event = login(game_version="1.99.0")

        protocol.on_login(event)

        event.cancel.assert_not_called()

    def test_translates_the_colour_codes_of_the_kick_message(self, make_protocol: Callable[..., BaseProtocol]) -> None:
        protocol = make_protocol('block-protocols = [975]\nblock-disconnect-msg = "&cGo away"\n')
        protocol.transform_serverbound(handshake(975))
        event = login()

        protocol.on_login(event)

        assert event.kick_message == "§cGo away"

    def test_says_nothing_about_a_refused_join_by_default(
        self, make_protocol: Callable[..., BaseProtocol], mock_logger: MagicMock
    ) -> None:
        protocol = make_protocol("block-protocols = [975]\n")
        protocol.transform_serverbound(handshake(975))

        protocol.on_login(login())

        mock_logger.info.assert_not_called()

    def test_reports_a_refused_join_when_asked_to(
        self, make_protocol: Callable[..., BaseProtocol], mock_logger: MagicMock
    ) -> None:
        protocol = make_protocol("block-protocols = [975]\n[logging]\nlog-blocked-joins = true\n")
        protocol.transform_serverbound(handshake(975))

        protocol.on_login(login())

        mock_logger.info.assert_called_once_with(f"Blocked join due to unsupported version from {ADDRESS} (26.20)")
