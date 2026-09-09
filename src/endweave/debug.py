"""Debug controls for packet translation.

Folds ViaVersion's DebugHandler interface and its implementation into a single
class: a master switch, the pre/post transform logging phases, and a packet
filter that matches either a packet type name across both directions or an
exact packet type within one direction.

See Also:
    com.viaversion.viaversion.api.debug.DebugHandler
    com.viaversion.viaversion.debug.DebugHandlerImpl
"""

import traceback
from dataclasses import dataclass
from enum import Enum
from typing import Protocol

from endstone import Logger


class Direction(Enum):
    """Whether a packet travels towards the server or towards the client.

    See Also:
        com.viaversion.viaversion.api.protocol.packet.Direction
    """

    SERVERBOUND = "serverbound"
    CLIENTBOUND = "clientbound"


@dataclass(frozen=True)
class PacketType:
    """A packet identified by ID and by a name that stays stable across versions.

    Bedrock uses one packet ID space for every connection state, so ViaVersion's
    State component has no counterpart here.

    Attributes:
        packet_id: Bedrock packet ID.
        name: Name kept consistent across protocol versions, e.g. "START_GAME".
        direction: Direction the packet travels in.

    See Also:
        com.viaversion.viaversion.api.protocol.packet.PacketType
    """

    packet_id: int
    name: str
    direction: Direction


class LoggablePacket(Protocol):
    """Structural view of the packet ``should_log`` filters on.

    Anything carrying a packet ID and an optional resolved packet type
    satisfies it.

    Attributes:
        packet_id: Bedrock packet ID read off the wire.
        packet_type: Resolved packet type, or None when the ID is unknown.
    """

    @property
    def packet_id(self) -> int: ...

    @property
    def packet_type(self) -> PacketType | None: ...


class DebugHandler:
    """Master switch and packet filter for translation debug logging.

    Attributes:
        enabled: Whether debug mode is on. Callers gate their logging on this;
            ``should_log`` only applies the packet filter.
        log_pre_packet_transform: Log packets before they are transformed.
        log_post_packet_transform: Log packets after they are transformed.
        log_conversion_warnings: Log conversion errors even with debug off.

    See Also:
        com.viaversion.viaversion.api.debug.DebugHandler
        com.viaversion.viaversion.debug.DebugHandlerImpl
    """

    def __init__(
        self,
        logger: Logger,
        *,
        enabled: bool = False,
        log_conversion_warnings: bool = True,
    ) -> None:
        self._logger = logger
        self.enabled = enabled
        self.log_pre_packet_transform = True
        self.log_post_packet_transform = False
        self.log_conversion_warnings = log_conversion_warnings
        self._packet_type_names: set[str] = set()
        self._packet_types: dict[Direction, set[PacketType]] = {direction: set() for direction in Direction}

    def add_packet_type_name_to_log(self, packet_type_name: str) -> None:
        """Log every packet with this type name, whichever direction it travels.

        Args:
            packet_type_name: Packet type name, e.g. "START_GAME".
        """
        self._packet_type_names.add(packet_type_name)

    def add_packet_type_to_log(self, packet_type: PacketType) -> None:
        """Log this packet type in its own direction.

        Packets are checked on each protocol transformer, so this is best used
        on single protocol pipes.

        Args:
            packet_type: Packet type to log.
        """
        self._packet_types[packet_type.direction].add(packet_type)

    def remove_packet_type_name_to_log(self, packet_type_name: str) -> bool:
        """Stop logging packets with this type name.

        Args:
            packet_type_name: Packet type name to drop from the filter.

        Returns:
            Whether the name was in the filter.
        """
        removed = packet_type_name in self._packet_type_names
        self._packet_type_names.discard(packet_type_name)
        return removed

    def remove_packet_type_to_log(self, packet_type: PacketType) -> bool:
        """Stop logging this packet type.

        Args:
            packet_type: Packet type to drop from the filter.

        Returns:
            Whether the packet type was in the filter.
        """
        packet_types = self._packet_types[packet_type.direction]
        removed = packet_type in packet_types
        packet_types.discard(packet_type)
        return removed

    def clear_packet_types_to_log(self) -> None:
        """Reset the packet filter, so every packet is logged again."""
        self._packet_type_names.clear()
        for packet_types in self._packet_types.values():
            packet_types.clear()

    def set_log_packet_transform(self, log_packet_transform: bool) -> None:
        """Set both the pre and post transform logging phases at once.

        Args:
            log_packet_transform: Whether packets should be logged before and
                after being transformed.
        """
        self.log_pre_packet_transform = log_packet_transform
        self.log_post_packet_transform = log_packet_transform

    def should_log(self, packet: LoggablePacket, direction: Direction) -> bool:
        """Check whether a packet passes the filter.

        An empty filter passes everything. A packet whose type could not be
        resolved is matched on its ID against the packet types registered for
        its direction.

        Args:
            packet: The packet being translated.
            direction: Direction the packet travels in.

        Returns:
            Whether the packet should be logged.
        """
        if not self._packet_type_names and not any(self._packet_types.values()):
            return True

        packet_types = self._packet_types[direction]
        packet_type = packet.packet_type
        if packet_type is not None:
            return packet_type.name in self._packet_type_names or packet_type in packet_types

        return any(known.packet_id == packet.packet_id for known in packet_types)

    def enable_and_log_types(self, *packet_types: PacketType) -> None:
        """Turn debug mode on and log only the given packet types.

        Args:
            *packet_types: Packet types to add to the filter.
        """
        self.enabled = True
        for packet_type in packet_types:
            self.add_packet_type_to_log(packet_type)

    def error(self, error: str, exception: BaseException) -> None:
        """Log a translation error, unless conversion warnings are muted.

        Args:
            error: Message describing what failed.
            exception: The exception that was raised.
        """
        if self.log_conversion_warnings or self.enabled:
            trace = "".join(traceback.format_exception(type(exception), exception, exception.__traceback__))
            self._logger.error(f"{error}\n{trace}")
