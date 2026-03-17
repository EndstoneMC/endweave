"""Per-player session tracking for protocol translation."""

from __future__ import annotations

from dataclasses import dataclass, field

from endstone import Player


@dataclass
class PlayerSession:
    """Tracks a connected player's protocol state."""

    address: str  # "host:port" key
    client_protocol: int = 0  # Detected from RequestNetworkSettings
    server_protocol: int = 924
    warned_no_translator: bool = False

    @property
    def needs_translation(self) -> bool:
        return self.client_protocol != 0 and self.client_protocol != self.server_protocol


class SessionManager:
    """Manages player sessions keyed by network address."""

    def __init__(self) -> None:
        self._sessions: dict[str, PlayerSession] = {}

    def get_or_create(self, address: str) -> PlayerSession:
        if address not in self._sessions:
            self._sessions[address] = PlayerSession(address=address)
        return self._sessions[address]

    def get(self, address: str) -> PlayerSession | None:
        return self._sessions.get(address)

    def remove_by_address(self, address: str) -> None:
        self._sessions.pop(address, None)

    def remove_by_player(self, player: Player) -> None:
        """Remove session by player object (uses player.address)."""
        self._sessions.pop(str(player.address), None)
