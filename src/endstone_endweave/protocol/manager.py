"""Protocol manager mapping version pairs to protocols."""

from __future__ import annotations

from collections import deque

from endstone_endweave.protocol.base import Protocol


class ProtocolManager:
    """Maps (server_protocol, client_protocol) -> Protocol.

    Supports chaining protocols for multi-step version gaps,
    inspired by ViaVersion's ProtocolManagerImpl.getProtocolPath().
    """

    def __init__(self) -> None:
        self._protocols: dict[tuple[int, int], Protocol] = {}
        self._base_protocols: list[Protocol] = []
        self._path_cache: dict[tuple[int, int], list[Protocol] | None] = {}

    def register(self, protocol: Protocol) -> None:
        key = (protocol.server_protocol, protocol.client_protocol)
        self._protocols[key] = protocol
        self._path_cache.clear()

    def register_base(self, protocol: Protocol) -> None:
        """Register a base protocol (always-on, runs before version-specific ones)."""
        self._base_protocols.append(protocol)

    def get_base_protocols(self) -> list[Protocol]:
        """Return all registered base protocols."""
        return self._base_protocols

    def get(self, server_protocol: int, client_protocol: int) -> Protocol | None:
        """Get a single direct protocol for a version pair."""
        return self._protocols.get((server_protocol, client_protocol))

    def get_path(
        self, server_protocol: int, client_protocol: int
    ) -> list[Protocol] | None:
        """Return ordered list of protocols to chain, or None if no path exists.

        Direct lookup first, then BFS for multi-step chains.
        Results are cached.
        """
        if server_protocol == client_protocol:
            return []

        cache_key = (server_protocol, client_protocol)
        if cache_key in self._path_cache:
            return self._path_cache[cache_key]

        # Direct lookup
        direct = self._protocols.get(cache_key)
        if direct is not None:
            result = [direct]
            self._path_cache[cache_key] = result
            return result

        # BFS from client_protocol toward server_protocol
        # Each protocol steps from client_protocol -> server_protocol,
        # so we search: starting at client_protocol, find edges where
        # client_protocol matches, and follow to server_protocol.
        path = self._bfs(server_protocol, client_protocol)
        self._path_cache[cache_key] = path
        return path

    def _bfs(
        self, server_protocol: int, client_protocol: int
    ) -> list[Protocol] | None:
        """BFS to find a chain of protocols from client -> server."""
        # Build adjacency: for each protocol (s, c), from c we can reach s
        adjacency: dict[int, list[Protocol]] = {}
        for (s, c), protocol in self._protocols.items():
            adjacency.setdefault(c, []).append(protocol)

        visited: set[int] = {client_protocol}
        queue: deque[tuple[int, list[Protocol]]] = deque()
        queue.append((client_protocol, []))

        while queue:
            current, path = queue.popleft()
            for protocol in adjacency.get(current, []):
                next_proto = protocol.server_protocol
                if next_proto == server_protocol:
                    return path + [protocol]
                if next_proto not in visited:
                    visited.add(next_proto)
                    queue.append((next_proto, path + [protocol]))

        return None

    def get_max_client_version(self, server_protocol: int) -> int | None:
        """Return the highest client protocol reachable from server_protocol."""
        max_version: int | None = None
        # Check all registered client protocols for reachability
        client_protocols = {c for _, c in self._protocols}
        for client in client_protocols:
            if self.get_path(server_protocol, client) is not None:
                if max_version is None or client > max_version:
                    max_version = client
        return max_version
