"""Protocol translation infrastructure."""

from __future__ import annotations

from collections.abc import Callable

from endstone_endweave.codec.wrapper import PacketWrapper
from endstone_endweave.protocol.direction import Direction

PacketHandler = Callable[[PacketWrapper], None]


class Protocol:
    """Translates packets between two protocol versions.

    Handlers are registered per packet ID and direction.
    Unregistered packets pass through unchanged.

    Handlers receive a PacketWrapper and use passthrough/read/write/cancel
    for field-level transforms. Access the connection via wrapper.user().
    """

    def __init__(self, server_protocol: int, client_protocol: int) -> None:
        self.server_protocol = server_protocol
        self.client_protocol = client_protocol
        self._handlers: dict[Direction, dict[int, PacketHandler]] = {
            Direction.CLIENTBOUND: {},
            Direction.SERVERBOUND: {},
        }
        self._cancel: dict[Direction, set[int]] = {
            Direction.CLIENTBOUND: set(),
            Direction.SERVERBOUND: set(),
        }

    def register_clientbound(self, packet_id: int, handler: PacketHandler) -> None:
        self._handlers[Direction.CLIENTBOUND][packet_id] = handler

    def register_serverbound(self, packet_id: int, handler: PacketHandler) -> None:
        self._handlers[Direction.SERVERBOUND][packet_id] = handler

    def cancel_clientbound(self, *packet_ids: int) -> None:
        self._cancel[Direction.CLIENTBOUND].update(packet_ids)

    def cancel_serverbound(self, *packet_ids: int) -> None:
        self._cancel[Direction.SERVERBOUND].update(packet_ids)

    def transform(
        self, direction: Direction, packet_id: int, wrapper: PacketWrapper
    ) -> None:
        """Run the handler for this packet in the given direction, if any."""
        if packet_id in self._cancel[direction]:
            wrapper.cancel()
            return
        handler = self._handlers[direction].get(packet_id)
        if handler is not None:
            handler(wrapper)
