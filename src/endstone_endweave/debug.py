"""Debug packet logging with per-packet-type filtering.

Supports pre/post transform logging phases, packet type filtering,
and a structured log format.

See Also:
    com.viaversion.viaversion.api.debug.DebugHandler
"""

from endstone import Logger

from endstone_endweave.protocol.packet_ids import PacketId


def packet_label(packet_id: int) -> str:
    """Format a packet ID as 'NAME(id) (0xHH)' for debug display.

    Args:
        packet_id: Bedrock packet ID.

    Returns:
        e.g. 'START_GAME(11) (0x0B)' or '999 (0x03E7)'.

    See Also:
        com.viaversion.viaversion.util.ProtocolUtil
    """
    hex_str = format(packet_id, "X")
    nice_hex = ("0x0" if len(hex_str) == 1 else "0x") + hex_str
    try:
        name = PacketId(packet_id).name
        return f"{name}({packet_id}) ({nice_hex})"
    except ValueError:
        return f"{packet_id} ({nice_hex})"


class DebugHandler:
    """Controls debug logging with optional packet ID filtering.

    Provides a master enabled flag, separate pre/post transform logging
    phases, and a packet ID filter set (empty = log all).

    Attributes:
        _logger: Endstone logger instance.
        _enabled: Whether debug logging is active.
        _log_pre: Log packets before transformation (default True when enabled).
        _log_post: Log packets after transformation (default False).
        _filter: Set of packet IDs to log (empty = all).

    See Also:
        com.viaversion.viaversion.api.debug.DebugHandler
    """

    def __init__(
        self,
        logger: Logger,
        enabled: bool = False,
        packets: frozenset[int] = frozenset(),
        log_pre: bool = True,
        log_post: bool = False,
    ) -> None:
        self._logger = logger
        self._enabled = enabled
        self._log_pre = log_pre
        self._log_post = log_post
        self._filter: frozenset[int] = frozenset(packets)

    @property
    def enabled(self) -> bool:
        return self._enabled

    @property
    def log_pre_packet_transform(self) -> bool:
        return self._log_pre

    @property
    def log_post_packet_transform(self) -> bool:
        return self._log_post

    def should_log(self, packet_id: int) -> bool:
        """Check if this packet ID passes the filter.

        Args:
            packet_id: Bedrock packet ID to check.
        """
        if not self._enabled:
            return False
        if not self._filter:
            return True
        return packet_id in self._filter

    def log_packet(
        self,
        stage: str,
        address: str,
        direction: str,
        state: str,
        packet_id: int,
        client_version: int,
        size: int,
    ) -> None:
        """Log a packet event in structured format.

        Format: {STAGE}: {ADDRESS} {DIRECTION} {STATE}: {PACKET_LABEL} [{VERSION}] {SIZE}b

        Args:
            stage: "PRE " or "POST" (padded to 4 chars).
            address: Connection address string.
            direction: "SERVERBOUND" or "CLIENTBOUND".
            state: Connection state (e.g. "LOGIN", "PLAY").
            packet_id: Bedrock packet ID.
            client_version: Client protocol version number.
            size: Payload size in bytes.

        See Also:
            com.viaversion.viaversion.protocol.ProtocolPipelineImpl#logPacket
        """
        if not self.should_log(packet_id):
            return
        label = packet_label(packet_id)
        self._logger.debug(f"{stage}: {address} {direction} {state}: {label} [{client_version}] {size}b")

    def log(self, packet_id: int, message: str) -> None:
        """Log a debug message if the packet ID passes the filter.

        Args:
            packet_id: Bedrock packet ID triggering the log.
            message: The debug message to log.
        """
        if self.should_log(packet_id):
            self._logger.debug(message)

    @staticmethod
    def from_config(logger: Logger, config: dict[str, object]) -> "DebugHandler":
        """Create a DebugHandler from a plugin config dict.

        Args:
            logger: Endstone logger instance.
            config: The full plugin config dict.

        Returns:
            Configured DebugHandler instance.
        """
        debug_cfg: dict[str, object] = config.get("debug", {}) or {}  # type: ignore[assignment]
        enabled = bool(debug_cfg.get("enabled", False))
        packets: list[int] = debug_cfg.get("packets", []) or []  # type: ignore[assignment]
        log_post = bool(debug_cfg.get("log_post_transform", False))
        return DebugHandler(
            logger,
            enabled=enabled,
            packets=frozenset(packets),
            log_pre=True,
            log_post=log_post,
        )
