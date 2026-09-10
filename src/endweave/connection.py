"""The connections Endweave tracks, and the registry holding them.

ViaVersion's UserConnection and ConnectionManager, down to what a Bedrock
server plugin can hold: the peer, the protocol it speaks against the one the
server speaks, and whether the connection is still live.

Connections are keyed by address string and created on the first packet from a
peer, not at login. Endstone gives a packet event an address, a sub client id
and no player until login finishes, so across the login sequence the address is
the only identity to hand, where ViaVersion has a uuid from LOGIN_SUCCESS
onwards. Two things that key cannot do: tell split screen clients on one
address apart, and identify a NetherNet peer, whose address comes through empty
because BDS holds its identity as a NetherNet id rather than an address, so no
NetherNet peer is tracked. Both wait on Endstone exposing the NetworkIdentifier
itself. A peer that shakes hands and never logs in is never quit either, so
pending connections are capped and the oldest are dropped.

ProtocolInfo is folded in, less its connection state, which Bedrock has no
counterpart for, and its compression flag, which Endstone handles below this.
Its pipeline is a pair of translators instead of a chain of Protocol objects,
since the engine resolves the whole path from one version to another at compile
time. The StorableObject storage a transform reads lives in the engine's
Session, which the two directions share. The client side of a connection is
gone too: ViaVersion draws that line for ViaProxy, which runs Via as a client,
where an Endstone plugin is always the server.

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

from endstone import Player

from . import _pipeline
from .protocol.version import UNKNOWN, ProtocolVersion

__all__ = ["MAX_PENDING_CONNECTIONS", "Connection", "ConnectionManager"]

MAX_PENDING_CONNECTIONS = 1024

_IDS = itertools.count(1)


class Connection:
    """One peer's connection and the protocol versions on either end of it.

    ``serverbound`` and ``clientbound`` are the pipeline the base protocol
    installs on the handshake: a translator each way, or None where the two
    ends already agree or the engine does not carry one of them.

    See Also:
        com.viaversion.viaversion.api.connection.UserConnection
        com.viaversion.viaversion.connection.UserConnectionImpl
    """

    def __init__(self, address: str, server_protocol_version: ProtocolVersion) -> None:
        self._id = next(_IDS)
        self._address = address
        self._server_protocol_version = server_protocol_version
        self.protocol_version = UNKNOWN
        self.session = _pipeline.Session()
        self.serverbound: _pipeline.Translator | None = None
        self.clientbound: _pipeline.Translator | None = None
        self.player: Player | None = None
        self.active = True
        self.pending_disconnect = False

    @property
    def id(self) -> int:
        return self._id

    @property
    def address(self) -> str:
        return self._address

    @property
    def server_protocol_version(self) -> ProtocolVersion:
        return self._server_protocol_version

    def disconnect(self, reason: str) -> None:
        if self.player is None or not self.active or self.pending_disconnect:
            return

        self.pending_disconnect = True
        self.player.kick(reason)

    def __repr__(self) -> str:
        return f"Connection(id={self._id}, address={self._address!r}, protocol_version={self.protocol_version!r})"


class ConnectionManager:
    """The connections being handled, keyed by the peer's address.

    See Also:
        com.viaversion.viaversion.api.connection.ConnectionManager
        com.viaversion.viaversion.connection.ConnectionManagerImpl
    """

    def __init__(self, server_protocol_version: ProtocolVersion) -> None:
        self._server_protocol_version = server_protocol_version
        self._connections: dict[str, Connection] = {}
        self._connections_view = MappingProxyType(self._connections)

    @property
    def connections(self) -> Mapping[str, Connection]:
        return self._connections_view

    def has_connection(self, address: str) -> bool:
        return address in self._connections

    def get_connection(self, address: str) -> Connection | None:
        return self._connections.get(address)

    def get_or_create(self, address: str) -> Connection:
        connection = self._connections.get(address)
        if connection is not None:
            return connection

        connection = Connection(address, self._server_protocol_version)
        self._connections[address] = connection

        pending = [tracked for tracked in self._connections.values() if tracked.player is None]
        for stale in pending[:-MAX_PENDING_CONNECTIONS]:
            del self._connections[stale.address]
        return connection

    def on_disconnect(self, connection: Connection) -> None:
        connection.active = False
        if self._connections.get(connection.address) is connection:
            del self._connections[connection.address]
