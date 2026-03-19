"""Concrete protocol translation infrastructure."""

from __future__ import annotations

from collections.abc import Callable

from endstone_endweave.codec.wrapper import PacketWrapper
from endstone_endweave.connection import UserConnection

PacketHandler = Callable[[PacketWrapper, UserConnection], None]


class Protocol:
    """Translates packets between two protocol versions.

    Handlers are registered per packet ID and direction.
    Unregistered packets pass through unchanged.

    Handlers receive a PacketWrapper and use passthrough/read/write/cancel
    for field-level transforms.
    """

    def __init__(self, server_protocol: int, client_protocol: int) -> None:
        self.server_protocol = server_protocol
        self.client_protocol = client_protocol
        self._clientbound: dict[int, PacketHandler] = {}
        self._serverbound: dict[int, PacketHandler] = {}
        self._cancel_clientbound: set[int] = set()
        self._cancel_serverbound: set[int] = set()

    def register_clientbound(self, packet_id: int, handler: PacketHandler) -> None:
        self._clientbound[packet_id] = handler

    def register_serverbound(self, packet_id: int, handler: PacketHandler) -> None:
        self._serverbound[packet_id] = handler

    def cancel_clientbound(self, *packet_ids: int) -> None:
        self._cancel_clientbound.update(packet_ids)

    def cancel_serverbound(self, *packet_ids: int) -> None:
        self._cancel_serverbound.update(packet_ids)

    def transform_clientbound(
        self, packet_id: int, wrapper: PacketWrapper, connection: UserConnection
    ) -> None:
        """Run the clientbound handler for this packet, if any."""
        if packet_id in self._cancel_clientbound:
            wrapper.cancel()
            return
        handler = self._clientbound.get(packet_id)
        if handler is not None:
            handler(wrapper, connection)

    def transform_serverbound(
        self, packet_id: int, wrapper: PacketWrapper, connection: UserConnection
    ) -> None:
        """Run the serverbound handler for this packet, if any."""
        if packet_id in self._cancel_serverbound:
            wrapper.cancel()
            return
        handler = self._serverbound.get(packet_id)
        if handler is not None:
            handler(wrapper, connection)
