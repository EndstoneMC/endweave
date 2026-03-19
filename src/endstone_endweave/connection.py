"""Per-player connection tracking for protocol translation."""


from dataclasses import dataclass, field
from typing import TYPE_CHECKING

from endstone import Logger, Player

if TYPE_CHECKING:
    from endstone_endweave.protocol import Protocol


@dataclass
class UserConnection:
    """Tracks a connected player's protocol state."""

    address: str  # "host:port" key
    logger: Logger = field(repr=False)
    client_protocol: int = 0  # Detected from RequestNetworkSettings
    server_protocol: int = 0  # Set by ConnectionManager
    warned_no_chain: bool = False
    protocol_chain: "list[Protocol] | None" = None  # cached after first lookup

    @property
    def needs_translation(self) -> bool:
        return (
            self.client_protocol != 0 and self.client_protocol != self.server_protocol
        )


class ConnectionManager:
    """Manages player connections keyed by network address."""

    def __init__(self, server_protocol: int = 0, logger: Logger | None = None) -> None:
        self._server_protocol = server_protocol
        self._logger = logger
        self._connections: dict[str, UserConnection] = {}

    def get_or_create(self, address: str) -> UserConnection:
        if address not in self._connections:
            self._connections[address] = UserConnection(
                address=address,
                logger=self._logger,  # type: ignore[arg-type]
                server_protocol=self._server_protocol,
            )
        return self._connections[address]

    def get(self, address: str) -> UserConnection | None:
        return self._connections.get(address)

    def remove_by_address(self, address: str) -> None:
        self._connections.pop(address, None)

    def remove_by_player(self, player: Player) -> None:
        """Remove connection by player object (uses player.address)."""
        self._connections.pop(str(player.address), None)
