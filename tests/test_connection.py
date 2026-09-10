"""Connection identity and lifecycle, and the registry that tracks them by address."""

from __future__ import annotations

from unittest.mock import MagicMock

import pytest

from endweave.connection import MAX_PENDING_CONNECTIONS, Connection, ConnectionManager
from endweave.protocol.version import UNKNOWN, get_protocol

CLIENT = get_protocol(975)
SERVER = get_protocol(944)
ADDRESS = "127.0.0.1:19132"

# The pair the engine carries, plus the version that is wire-identical to the older one.
CARRIED_SERVER = get_protocol(2168)
CARRIED_CLIENT = get_protocol(2192)
CARRIED_ALIAS = get_protocol(2169)


@pytest.fixture
def manager() -> ConnectionManager:
    return ConnectionManager(SERVER)


class TestConnection:
    def test_starts_unknown_and_unclaimed(self) -> None:
        connection = Connection(ADDRESS, SERVER)

        assert connection.address == ADDRESS
        assert connection.server_protocol_version == SERVER
        assert connection.protocol_version is UNKNOWN
        assert connection.player is None
        assert connection.active is True
        assert connection.pending_disconnect is False

    def test_every_connection_gets_its_own_id(self) -> None:
        assert Connection(ADDRESS, SERVER).id < Connection(ADDRESS, SERVER).id

    def test_disconnect_kicks_the_player(self) -> None:
        connection = Connection(ADDRESS, SERVER)
        connection.player = MagicMock()

        connection.disconnect("§cUnsupported version")

        assert connection.pending_disconnect is True
        connection.player.kick.assert_called_once_with("§cUnsupported version")

    def test_disconnect_kicks_only_once(self) -> None:
        connection = Connection(ADDRESS, SERVER)
        connection.player = MagicMock()

        connection.disconnect("first")
        connection.disconnect("second")

        connection.player.kick.assert_called_once_with("first")

    def test_disconnect_does_nothing_before_a_player_is_attached(self) -> None:
        connection = Connection(ADDRESS, SERVER)

        connection.disconnect("too early")

        assert connection.pending_disconnect is False

    def test_disconnect_does_nothing_once_the_player_is_gone(self) -> None:
        connection = Connection(ADDRESS, SERVER)
        connection.player = MagicMock()
        connection.active = False

        connection.disconnect("too late")

        assert connection.pending_disconnect is False
        connection.player.kick.assert_not_called()

    def test_repr_names_the_address(self) -> None:
        assert ADDRESS in repr(Connection(ADDRESS, SERVER))


class TestPipeline:
    def test_carries_nothing_before_the_client_states_its_version(self) -> None:
        connection = Connection(ADDRESS, CARRIED_SERVER)

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_resolves_a_translator_for_each_direction(self) -> None:
        connection = Connection(ADDRESS, CARRIED_SERVER)

        connection.protocol_version = CARRIED_CLIENT

        assert connection.serverbound is not None
        assert (connection.serverbound.from_version, connection.serverbound.to_version) == (2192, 2168)
        assert connection.clientbound is not None
        assert (connection.clientbound.from_version, connection.clientbound.to_version) == (2168, 2192)

    def test_carries_nothing_when_both_ends_agree(self) -> None:
        connection = Connection(ADDRESS, CARRIED_SERVER)

        connection.protocol_version = CARRIED_SERVER

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_reads_a_wire_identical_client_as_the_version_it_speaks(self) -> None:
        connection = Connection(ADDRESS, CARRIED_CLIENT)

        connection.protocol_version = CARRIED_ALIAS

        assert connection.serverbound is not None
        assert connection.serverbound.from_version == 2168

    def test_leaves_a_wire_identical_client_alone_on_the_version_it_speaks(self) -> None:
        connection = Connection(ADDRESS, CARRIED_SERVER)

        connection.protocol_version = CARRIED_ALIAS

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_carries_nothing_for_a_client_the_engine_does_not_know(self) -> None:
        connection = Connection(ADDRESS, CARRIED_SERVER)

        connection.protocol_version = CLIENT

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_carries_nothing_for_a_server_the_engine_does_not_know(self) -> None:
        connection = Connection(ADDRESS, SERVER)

        connection.protocol_version = CARRIED_CLIENT

        assert connection.serverbound is None
        assert connection.clientbound is None

    def test_re_resolves_when_the_client_version_changes(self) -> None:
        connection = Connection(ADDRESS, CARRIED_SERVER)
        connection.protocol_version = CARRIED_CLIENT

        connection.protocol_version = CLIENT

        assert connection.serverbound is None
        assert connection.clientbound is None


class TestTracking:
    def test_creates_a_connection_for_a_new_address(self, manager: ConnectionManager) -> None:
        connection = manager.get_or_create(ADDRESS)

        assert connection.address == ADDRESS
        assert connection.server_protocol_version == SERVER
        assert manager.has_connection(ADDRESS) is True
        assert manager.get_connection(ADDRESS) is connection

    def test_returns_the_same_connection_for_a_known_address(self, manager: ConnectionManager) -> None:
        connection = manager.get_or_create(ADDRESS)
        connection.protocol_version = CLIENT

        assert manager.get_or_create(ADDRESS) is connection
        assert manager.get_or_create(ADDRESS).protocol_version == CLIENT

    def test_an_unknown_address_has_no_connection(self, manager: ConnectionManager) -> None:
        assert manager.has_connection(ADDRESS) is False
        assert manager.get_connection(ADDRESS) is None


class TestDisconnecting:
    def test_a_peer_leaving_is_dropped_and_marked_inactive(self, manager: ConnectionManager) -> None:
        connection = manager.get_or_create(ADDRESS)

        manager.on_disconnect(connection)

        assert connection.active is False
        assert manager.has_connection(ADDRESS) is False

    def test_an_untracked_connection_is_still_marked_inactive(self, manager: ConnectionManager) -> None:
        connection = Connection(ADDRESS, SERVER)

        manager.on_disconnect(connection)

        assert connection.active is False
        assert manager.connections == {}

    def test_a_stale_connection_leaves_the_newer_one_alone(self, manager: ConnectionManager) -> None:
        first = manager.get_or_create(ADDRESS)
        manager.on_disconnect(first)
        second = manager.get_or_create(ADDRESS)

        manager.on_disconnect(first)

        assert manager.get_connection(ADDRESS) is second


class TestPendingCap:
    def test_drops_the_oldest_peer_that_never_logged_in(self, manager: ConnectionManager) -> None:
        for port in range(MAX_PENDING_CONNECTIONS + 1):
            manager.get_or_create(f"127.0.0.1:{port}")

        assert len(manager.connections) == MAX_PENDING_CONNECTIONS
        assert manager.has_connection("127.0.0.1:0") is False
        assert manager.has_connection(f"127.0.0.1:{MAX_PENDING_CONNECTIONS}") is True

    def test_keeps_a_peer_that_has_a_player(self, manager: ConnectionManager) -> None:
        first = manager.get_or_create("127.0.0.1:0")
        first.player = MagicMock()

        for port in range(1, MAX_PENDING_CONNECTIONS + 2):
            manager.get_or_create(f"127.0.0.1:{port}")

        assert manager.get_connection("127.0.0.1:0") is first
        assert manager.has_connection("127.0.0.1:1") is False


class TestConnectionsView:
    def test_follows_the_registry(self, manager: ConnectionManager) -> None:
        view = manager.connections
        connection = manager.get_or_create(ADDRESS)

        assert dict(view) == {ADDRESS: connection}

    def test_cannot_be_written_through(self, manager: ConnectionManager) -> None:
        with pytest.raises(TypeError):
            manager.connections[ADDRESS] = Connection(ADDRESS, SERVER)  # type: ignore[index]
