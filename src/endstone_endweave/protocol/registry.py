"""Registry mapping protocol version pairs to translators."""

from __future__ import annotations

from collections import deque

from endstone_endweave.protocol.base import ProtocolTranslator


class TranslatorRegistry:
    """Maps (server_protocol, client_protocol) -> ProtocolTranslator.

    Supports chaining translators for multi-step version gaps,
    inspired by ViaVersion's ProtocolManagerImpl.getProtocolPath().
    """

    def __init__(self) -> None:
        self._translators: dict[tuple[int, int], ProtocolTranslator] = {}
        self._path_cache: dict[tuple[int, int], list[ProtocolTranslator] | None] = {}

    def register(self, translator: ProtocolTranslator) -> None:
        key = (translator.server_protocol, translator.client_protocol)
        self._translators[key] = translator
        self._path_cache.clear()

    def get(self, server_protocol: int, client_protocol: int) -> ProtocolTranslator | None:
        """Get a single direct translator for a version pair."""
        return self._translators.get((server_protocol, client_protocol))

    def get_path(
        self, server_protocol: int, client_protocol: int
    ) -> list[ProtocolTranslator] | None:
        """Return ordered list of translators to chain, or None if no path exists.

        Direct lookup first, then BFS for multi-step chains.
        Results are cached.
        """
        if server_protocol == client_protocol:
            return []

        cache_key = (server_protocol, client_protocol)
        if cache_key in self._path_cache:
            return self._path_cache[cache_key]

        # Direct lookup
        direct = self._translators.get(cache_key)
        if direct is not None:
            result = [direct]
            self._path_cache[cache_key] = result
            return result

        # BFS from client_protocol toward server_protocol
        # Each translator steps from client_protocol -> server_protocol,
        # so we search: starting at client_protocol, find edges where
        # client_protocol matches, and follow to server_protocol.
        path = self._bfs(server_protocol, client_protocol)
        self._path_cache[cache_key] = path
        return path

    def _bfs(
        self, server_protocol: int, client_protocol: int
    ) -> list[ProtocolTranslator] | None:
        """BFS to find a chain of translators from client -> server."""
        # Build adjacency: for each translator (s, c), from c we can reach s
        adjacency: dict[int, list[ProtocolTranslator]] = {}
        for (s, c), translator in self._translators.items():
            adjacency.setdefault(c, []).append(translator)

        visited: set[int] = {client_protocol}
        queue: deque[tuple[int, list[ProtocolTranslator]]] = deque()
        queue.append((client_protocol, []))

        while queue:
            current, path = queue.popleft()
            for translator in adjacency.get(current, []):
                next_proto = translator.server_protocol
                if next_proto == server_protocol:
                    return path + [translator]
                if next_proto not in visited:
                    visited.add(next_proto)
                    queue.append((next_proto, path + [translator]))

        return None

    def get_max_client_version(self, server_protocol: int) -> int | None:
        """Return the highest client protocol reachable from server_protocol."""
        max_version: int | None = None
        # Check all registered client protocols for reachability
        client_protocols = {c for _, c in self._translators}
        for client in client_protocols:
            if self.get_path(server_protocol, client) is not None:
                if max_version is None or client > max_version:
                    max_version = client
        return max_version
