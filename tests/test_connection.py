"""Connection identity and lifecycle, and the registry that tracks them."""

from __future__ import annotations

from unittest.mock import MagicMock
from uuid import UUID, uuid4

import pytest

from endweave.connection import Connection, ConnectionManager
from endweave.protocol.version import ProtocolVersion, get_protocol

CLIENT = get_protocol(975)
SERVER = get_protocol(944)


def make_player(name: str = "alice", unique_id: UUID | None = None) -> MagicMock:
    player = MagicMock()
    player.name = name
    player.unique_id = uuid4() if unique_id is None else unique_id
    return player


def make_connection(
    player: MagicMock | None = None,
    protocol_version: ProtocolVersion = CLIENT,
    server_protocol_version: ProtocolVersion = SERVER,
) -> Connection:
    return Connection(make_player() if player is None else player, protocol_version, server_protocol_version)


@pytest.fixture
def manager(mock_logger: MagicMock) -> ConnectionManager:
    return ConnectionManager(mock_logger)


class TestConnection:
    def test_carries_the_player_and_both_protocol_versions(self) -> None:
        player = make_player("bob")
        connection = make_connection(player)

        assert connection.player is player
        assert connection.name == "bob"
        assert connection.unique_id == player.unique_id
        assert connection.protocol_version == CLIENT
        assert connection.server_protocol_version == SERVER

    def test_starts_active_and_not_disconnecting(self) -> None:
        connection = make_connection()

        assert connection.active is True
        assert connection.pending_disconnect is False

    def test_every_connection_gets_its_own_id(self) -> None:
        first = make_connection()
        second = make_connection()

        assert second.id > first.id

    def test_keeps_the_name_and_uuid_the_player_had(self) -> None:
        player = make_player("bob")
        unique_id = player.unique_id
        connection = make_connection(player)

        player.name = "someone else"
        player.unique_id = uuid4()

        assert connection.name == "bob"
        assert connection.unique_id == unique_id

    def test_disconnect_kicks_the_player(self) -> None:
        player = make_player()
        connection = make_connection(player)

        connection.disconnect("§cUnsupported version")

        assert connection.pending_disconnect is True
        player.kick.assert_called_once_with("§cUnsupported version")

    def test_disconnect_kicks_only_once(self) -> None:
        player = make_player()
        connection = make_connection(player)

        connection.disconnect("first")
        connection.disconnect("second")

        player.kick.assert_called_once_with("first")

    def test_disconnect_does_nothing_once_the_player_is_gone(self) -> None:
        player = make_player()
        connection = make_connection(player)
        connection.active = False

        connection.disconnect("too late")

        assert connection.pending_disconnect is False
        player.kick.assert_not_called()

    def test_repr_names_the_connection(self) -> None:
        connection = make_connection(make_player("bob"))

        assert "bob" in repr(connection)
        assert str(CLIENT.version) in repr(connection)


class TestRegistering:
    def test_a_logged_in_player_is_tracked(self, manager: ConnectionManager) -> None:
        connection = make_connection()

        manager.on_login_success(connection)

        assert manager.has_connection(connection.unique_id) is True
        assert manager.get_connection(connection.unique_id) is connection

    def test_an_unknown_player_has_no_connection(self, manager: ConnectionManager) -> None:
        assert manager.has_connection(uuid4()) is False
        assert manager.get_connection(uuid4()) is None

    def test_a_connection_that_already_went_is_not_tracked(
        self, manager: ConnectionManager, mock_logger: MagicMock
    ) -> None:
        connection = make_connection()
        connection.active = False

        manager.on_login_success(connection)

        assert manager.connections == {}
        mock_logger.warning.assert_not_called()

    def test_registering_the_same_connection_twice_is_quiet(
        self, manager: ConnectionManager, mock_logger: MagicMock
    ) -> None:
        connection = make_connection()

        manager.on_login_success(connection)
        manager.on_login_success(connection)

        assert manager.get_connection(connection.unique_id) is connection
        mock_logger.warning.assert_not_called()

    def test_a_second_connection_for_one_uuid_warns_and_wins(
        self, manager: ConnectionManager, mock_logger: MagicMock
    ) -> None:
        player = make_player()
        first = make_connection(player)
        second = make_connection(make_player(unique_id=player.unique_id))

        manager.on_login_success(first)
        manager.on_login_success(second)

        assert manager.get_connection(player.unique_id) is second
        mock_logger.warning.assert_called_once_with(f"Duplicate UUID on connection! ({player.unique_id})")


class TestDisconnecting:
    def test_a_player_leaving_is_dropped_and_marked_inactive(self, manager: ConnectionManager) -> None:
        connection = make_connection()
        manager.on_login_success(connection)

        manager.on_disconnect(connection)

        assert connection.active is False
        assert manager.has_connection(connection.unique_id) is False

    def test_an_untracked_connection_is_still_marked_inactive(self, manager: ConnectionManager) -> None:
        connection = make_connection()

        manager.on_disconnect(connection)

        assert connection.active is False
        assert manager.connections == {}

    def test_a_stale_connection_leaves_the_newer_one_alone(self, manager: ConnectionManager) -> None:
        player = make_player()
        first = make_connection(player)
        second = make_connection(make_player(unique_id=player.unique_id))
        manager.on_login_success(first)
        manager.on_login_success(second)

        manager.on_disconnect(first)

        assert manager.get_connection(player.unique_id) is second


class TestConnectionsView:
    def test_follows_the_registry(self, manager: ConnectionManager) -> None:
        view = manager.connections
        connection = make_connection()
        manager.on_login_success(connection)

        assert dict(view) == {connection.unique_id: connection}

    def test_cannot_be_written_through(self, manager: ConnectionManager) -> None:
        with pytest.raises(TypeError):
            manager.connections[uuid4()] = make_connection()  # type: ignore[index]
