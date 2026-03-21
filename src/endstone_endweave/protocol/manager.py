"""Protocol manager mapping version pairs to protocols."""

from collections import deque

from endstone_endweave.protocol import Protocol


class ProtocolManager:
    """Maps (server_protocol, client_protocol) -> Protocol.

    Supports chaining protocols for multi-step version gaps
    using BFS to find the shortest translation path.

    Attributes:
        _protocols: Direct mapping from (server, client) version pair to Protocol.
        _base_protocols: Always-on protocols that run before version-specific ones.
        _path_cache: Cached BFS results mapping version pairs to protocol chains.

    See Also:
        com.viaversion.viaversion.protocol.ProtocolManagerImpl#getProtocolPath
    """

    def __init__(self) -> None:
        self._protocols: dict[tuple[int, int], Protocol] = {}
        self._base_protocols: list[Protocol] = []
        self._path_cache: dict[tuple[int, int], list[Protocol] | None] = {}

    def register(self, protocol: Protocol) -> None:
        """Register a version-specific protocol for its (server, client) pair.

        Args:
            protocol: Protocol to register. Clears the path cache.
        """
        key = (protocol.server_protocol, protocol.client_protocol)
        self._protocols[key] = protocol
        self._path_cache.clear()

    def register_base(self, protocol: Protocol) -> None:
        """Register a base protocol (always-on, runs before version-specific ones).

        Args:
            protocol: Base protocol to append to the base protocol list.
        """
        self._base_protocols.append(protocol)

    @property
    def base_protocols(self) -> list[Protocol]:
        """All registered base protocols."""
        return self._base_protocols

    def get(self, server_protocol: int, client_protocol: int) -> Protocol | None:
        """Get a single direct protocol for a version pair.

        Args:
            server_protocol: Server-side protocol number.
            client_protocol: Client-side protocol number.

        Returns:
            The registered Protocol, or None if no direct mapping exists.
        """
        return self._protocols.get((server_protocol, client_protocol))

    def get_path(self, server_protocol: int, client_protocol: int) -> list[Protocol] | None:
        """Return ordered list of protocols to chain, or None if no path exists.

        Direct lookup first, then BFS for multi-step chains.
        Results are cached.

        Args:
            server_protocol: Target server-side protocol number.
            client_protocol: Source client-side protocol number.

        Returns:
            Ordered list of Protocols forming the translation chain,
            an empty list if versions match, or None if unreachable.
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

    def _bfs(self, server_protocol: int, client_protocol: int) -> list[Protocol] | None:
        """BFS to find a chain of protocols from client -> server.

        Args:
            server_protocol: Target server-side protocol number.
            client_protocol: Source client-side protocol number.

        Returns:
            Ordered list of Protocols forming the shortest chain,
            or None if no path exists.
        """
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
        """Return the highest client protocol reachable from server_protocol.

        Args:
            server_protocol: Server-side protocol number to search from.

        Returns:
            The highest reachable client protocol number, or None if none found.
        """
        reachable = [c for _, c in self._protocols if self.get_path(server_protocol, c) is not None]
        return max(reachable) if reachable else None
