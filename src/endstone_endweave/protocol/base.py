"""Concrete protocol translation infrastructure."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass

from endstone_endweave.session import PlayerSession

PacketHandler = Callable[[bytes, PlayerSession], "PacketTransformation"]


@dataclass
class PacketTransformation:
    """Result of a packet rewrite operation."""

    cancel: bool = False
    new_payload: bytes | None = None


class ProtocolTranslator:
    """Translates packets between two protocol versions.

    Handlers are registered per packet ID and direction.
    Unregistered packets pass through unchanged.
    """

    def __init__(self, server_protocol: int, client_protocol: int) -> None:
        self.server_protocol = server_protocol
        self.client_protocol = client_protocol
        self._clientbound: dict[int, PacketHandler] = {}
        self._serverbound: dict[int, PacketHandler] = {}
        self._cancel_serverbound: set[int] = set()

    def register_clientbound(self, packet_id: int, handler: PacketHandler) -> None:
        self._clientbound[packet_id] = handler

    def register_serverbound(self, packet_id: int, handler: PacketHandler) -> None:
        self._serverbound[packet_id] = handler

    def cancel_serverbound(self, *packet_ids: int) -> None:
        self._cancel_serverbound.update(packet_ids)

    def translate_clientbound(
        self, packet_id: int, payload: bytes, session: PlayerSession
    ) -> PacketTransformation:
        handler = self._clientbound.get(packet_id)
        if handler:
            return handler(payload, session)
        return PacketTransformation()

    def translate_serverbound(
        self, packet_id: int, payload: bytes, session: PlayerSession
    ) -> PacketTransformation:
        if packet_id in self._cancel_serverbound:
            return PacketTransformation(cancel=True)
        handler = self._serverbound.get(packet_id)
        if handler:
            return handler(payload, session)
        return PacketTransformation()
