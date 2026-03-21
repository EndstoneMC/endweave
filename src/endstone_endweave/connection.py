"""Per-player connection tracking for protocol translation."""

from dataclasses import dataclass, field
from enum import Enum
from typing import TYPE_CHECKING, TypeVar

from endstone import Logger, Player

if TYPE_CHECKING:
    from endstone_endweave.protocol import Protocol

_T = TypeVar("_T")


class ConnectionState(Enum):
    """Lifecycle state of a player connection, adapted for Bedrock Edition.

    - HANDSHAKE: Initial state before version negotiation
    - LOGIN: After RequestNetworkSettings, during authentication
    - PLAY: After StartGame, in-game packets

    See Also:
        com.viaversion.viaversion.api.protocol.packet.State
    """

    HANDSHAKE = "handshake"
    LOGIN = "login"
    PLAY = "play"


@dataclass
class UserConnection:
    """Tracks a connected player's protocol state.

    Stores per-player translation context including version detection,
    connection lifecycle phase, and type-keyed storage for handler state.

    Attributes:
        address: Network address string ("host:port") used as the lookup key.
        logger: Endstone logger instance (excluded from repr).
        client_protocol: Protocol version detected from RequestNetworkSettings (0 until set).
        server_protocol: Protocol version of the server this player connected to.
        state: Current connection lifecycle state.
        active: Whether non-base protocols are applied.
        pending_disconnect: Set when disconnect detected, prevents further processing.
        warned_no_chain: Whether a missing-chain warning has already been logged.
        protocol_chain: Cached translation chain after first lookup, or None.
        _storage: Type-keyed storage for per-connection state (entity tracking, etc.).

    See Also:
        com.viaversion.viaversion.connection.UserConnectionImpl
    """

    address: str  # "host:port" key
    logger: Logger = field(repr=False)
    client_protocol: int = 0  # Detected from RequestNetworkSettings
    server_protocol: int = 0  # Set by ConnectionManager
    state: ConnectionState = ConnectionState.HANDSHAKE
    active: bool = False  # True once protocol chain is resolved
    pending_disconnect: bool = False
    warned_no_chain: bool = False
    protocol_chain: "list[Protocol] | None" = None  # cached after first lookup
    _storage: dict[type, object] = field(default_factory=dict, repr=False)

    @property
    def needs_translation(self) -> bool:
        return self.client_protocol != 0 and self.client_protocol != self.server_protocol

    def get(self, cls: type[_T]) -> _T | None:
        """Return stored object for the given type, or None.

        Args:
            cls: The type key to look up.
        """
        return self._storage.get(cls)  # type: ignore[return-value]

    def put(self, obj: object) -> None:
        """Store an object keyed by its type.

        Args:
            obj: The object to store. Its type (type(obj)) is used as the key.
        """
        self._storage[type(obj)] = obj

    def has(self, cls: type) -> bool:
        """Check if storage contains an object for the given type.

        Args:
            cls: The type key to check.
        """
        return cls in self._storage

    def remove(self, cls: type) -> None:
        """Remove the stored object for the given type (no-op if absent).

        Args:
            cls: The type key to remove.
        """
        self._storage.pop(cls, None)

    def clear_storage(self) -> None:
        """Remove all stored objects."""
        self._storage.clear()


class ConnectionManager:
    """Manages player connections keyed by network address.

    Attributes:
        _server_protocol: Default server protocol assigned to new connections.
        _logger: Endstone logger passed to each new UserConnection.
        _connections: Mapping of address strings to UserConnection instances.
    """

    def __init__(self, server_protocol: int = 0, logger: Logger | None = None) -> None:
        self._server_protocol = server_protocol
        self._logger = logger
        self._connections: dict[str, UserConnection] = {}

    def get_or_create(self, address: str) -> UserConnection:
        """Return existing connection for address, or create a new one.

        Args:
            address: Network address string ("host:port").

        Returns:
            The existing or newly created UserConnection for the address.
        """
        if address not in self._connections:
            self._connections[address] = UserConnection(
                address=address,
                logger=self._logger,  # type: ignore[arg-type]
                server_protocol=self._server_protocol,
            )
        return self._connections[address]

    def get(self, address: str) -> UserConnection | None:
        """Return the connection for address, or None if not found.

        Args:
            address: Network address string ("host:port").

        Returns:
            The UserConnection if present, otherwise None.
        """
        return self._connections.get(address)

    def remove_by_address(self, address: str) -> None:
        """Remove connection by network address (no-op if not found).

        Clears per-connection storage before removal.

        Args:
            address: Network address string ("host:port").
        """
        conn = self._connections.pop(address, None)
        if conn is not None:
            conn.clear_storage()

    def remove_by_player(self, player: Player) -> None:
        """Remove connection by player object (uses player.address).

        Clears per-connection storage before removal.

        Args:
            player: Endstone Player whose connection should be removed.
        """
        conn = self._connections.pop(str(player.address), None)
        if conn is not None:
            conn.clear_storage()
