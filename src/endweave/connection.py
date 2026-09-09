"""The connections Endweave tracks, and the registry holding them.

ViaVersion's UserConnection and ConnectionManager, down to what a Bedrock
server plugin can hold. ProtocolInfo is folded in, less its connection state,
which Bedrock has no counterpart for, and its pipeline and compression flag,
which belong to a translation layer this does not model. Nothing here
translates a packet, and the per connection StorableObject storage is left out.

The Channel is an Endstone Player, so the netty pieces went with it: raw sends,
passthrough tokens, the packet tracker, and the close-future listener the
manager registers to clean up after itself. A player leaving arrives as an
event instead. The client side of a connection is gone too. ViaVersion draws
that line for ViaProxy, which runs Via as a client, where an Endstone plugin is
always the server, so one map replaces the two ConnectionManagerImpl keeps.

See Also:
    com.viaversion.viaversion.api.connection.ConnectionManager
    com.viaversion.viaversion.api.connection.ProtocolInfo
    com.viaversion.viaversion.api.connection.UserConnection
    com.viaversion.viaversion.connection.ConnectionManagerImpl
    com.viaversion.viaversion.connection.ProtocolInfoImpl
    com.viaversion.viaversion.connection.UserConnectionImpl
"""

from __future__ import annotations

import itertools
from collections.abc import Mapping
from types import MappingProxyType
from uuid import UUID

from endstone import Logger, Player

from .protocol.version import ProtocolVersion

__all__ = ["Connection", "ConnectionManager"]

_IDS = itertools.count(1)


class Connection:
    """One player's connection and the protocol versions on either end of it.

    See Also:
        com.viaversion.viaversion.api.connection.UserConnection
        com.viaversion.viaversion.connection.UserConnectionImpl
    """

    def __init__(
        self,
        player: Player,
        protocol_version: ProtocolVersion,
        server_protocol_version: ProtocolVersion,
    ) -> None:
        self._id = next(_IDS)
        self._player = player
        self._unique_id = player.unique_id
        self._name = player.name
        self._protocol_version = protocol_version
        self._server_protocol_version = server_protocol_version
        self.active = True
        self.pending_disconnect = False

    @property
    def id(self) -> int:
        return self._id

    @property
    def player(self) -> Player:
        return self._player

    @property
    def unique_id(self) -> UUID:
        return self._unique_id

    @property
    def name(self) -> str:
        return self._name

    @property
    def protocol_version(self) -> ProtocolVersion:
        return self._protocol_version

    @property
    def server_protocol_version(self) -> ProtocolVersion:
        return self._server_protocol_version

    def disconnect(self, reason: str) -> None:
        if not self.active or self.pending_disconnect:
            return

        self.pending_disconnect = True
        self._player.kick(reason)

    def __repr__(self) -> str:
        return f"Connection(id={self._id}, name={self._name!r}, protocol_version={self._protocol_version!r})"


class ConnectionManager:
    """The connections being handled, keyed by the player's uuid.

    See Also:
        com.viaversion.viaversion.api.connection.ConnectionManager
        com.viaversion.viaversion.connection.ConnectionManagerImpl
    """

    def __init__(self, logger: Logger) -> None:
        self._logger = logger
        self._connections: dict[UUID, Connection] = {}
        self._connections_view = MappingProxyType(self._connections)

    @property
    def connections(self) -> Mapping[UUID, Connection]:
        return self._connections_view

    def has_connection(self, unique_id: UUID) -> bool:
        return unique_id in self._connections

    def get_connection(self, unique_id: UUID) -> Connection | None:
        return self._connections.get(unique_id)

    def on_login_success(self, connection: Connection) -> None:
        if not connection.active:
            return

        previous = self._connections.get(connection.unique_id)
        if previous is not None and previous is not connection:
            self._logger.warning(f"Duplicate UUID on connection! ({connection.unique_id})")
        self._connections[connection.unique_id] = connection

    def on_disconnect(self, connection: Connection) -> None:
        connection.active = False
        if self._connections.get(connection.unique_id) is connection:
            del self._connections[connection.unique_id]
