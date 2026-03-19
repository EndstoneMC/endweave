"""Packet translation pipeline - routes packets through the appropriate protocol."""

from __future__ import annotations

import traceback

from endstone import Logger
from endstone.event import PacketReceiveEvent, PacketSendEvent

from endstone_endweave.codec.wrapper import PacketWrapper
from endstone_endweave.connection import ConnectionManager
from endstone_endweave.protocol.base import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.manager import ProtocolManager


def _pname(packet_id: int) -> str:
    """Resolve packet ID to name, e.g. 'START_GAME(11)' or '999'."""
    try:
        return f"{PacketId(packet_id).name}({packet_id})"
    except ValueError:
        return str(packet_id)


class ProtocolPipeline:
    """Intercepts packet events and applies protocol translation.

    Like ViaVersion's ProtocolPipelineImpl, creates a single PacketWrapper
    and passes it through each protocol's transform method in sequence.
    """

    def __init__(
        self,
        manager: ProtocolManager,
        connections: ConnectionManager,
        logger: Logger,
    ) -> None:
        self._manager = manager
        self._connections = connections
        self._logger = logger

    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        """Handle a serverbound (client->server) packet."""
        address = str(event.address)
        packet_id = event.packet_id
        payload = event.payload

        # Run base protocols first (always, even before needs_translation check)
        connection = self._connections.get_or_create(address)
        wrapper = PacketWrapper(payload)

        for base in self._manager.get_base_protocols():
            base.transform_serverbound(packet_id, wrapper, connection)
            if wrapper.cancelled:
                event.cancel()
                return

        if not connection.needs_translation:
            # Apply any base protocol modifications before returning
            new_payload = wrapper.to_bytes()
            if new_payload != payload:
                event.payload = new_payload
            return

        self._logger.debug(f"[SB] {_pname(packet_id)} ({len(payload)}b)")

        chain = self._get_chain(connection)
        if chain is None:
            if not connection.warned_no_chain:
                connection.warned_no_chain = True
                self._logger.warning(
                    f"No protocol chain for server={connection.server_protocol} "
                    f"client={connection.client_protocol} from {address}"
                )
            # Still apply base protocol modifications
            new_payload = wrapper.to_bytes()
            if new_payload != payload:
                event.payload = new_payload
            return

        # Serverbound: apply chain in order (client -> server direction)
        for protocol in chain:
            try:
                protocol.transform_serverbound(packet_id, wrapper, connection)
            except Exception:
                self._logger.error(
                    f"[SB] {_pname(packet_id)} from {address} "
                    f"EXCEPTION:\n{traceback.format_exc()}"
                )
                return
            if wrapper.cancelled:
                self._logger.debug(f"[SB] {_pname(packet_id)} CANCELLED")
                event.cancel()
                return

        new_payload = wrapper.to_bytes()
        if new_payload != payload:
            self._logger.debug(
                f"[SB] {_pname(packet_id)} rewritten "
                f"{len(payload)}b -> {len(new_payload)}b"
            )
            event.payload = new_payload

    def on_packet_send(self, event: PacketSendEvent) -> None:
        """Handle a clientbound (server->client) packet."""
        address = str(event.address)

        connection = self._connections.get(address)
        if connection is None or not connection.needs_translation:
            return  # fast path

        chain = self._get_chain(connection)
        if chain is None:
            return

        packet_id = event.packet_id
        payload = event.payload
        wrapper = PacketWrapper(payload)

        self._logger.debug(f"[CB] {_pname(packet_id)} ({len(payload)}b)")

        # Clientbound: apply chain in reverse order (server -> client direction)
        for protocol in reversed(chain):
            try:
                protocol.transform_clientbound(packet_id, wrapper, connection)
            except Exception:
                self._logger.error(
                    f"[CB] {_pname(packet_id)} to {address} "
                    f"EXCEPTION:\n{traceback.format_exc()}"
                )
                return
            if wrapper.cancelled:
                self._logger.debug(f"[CB] {_pname(packet_id)} CANCELLED")
                event.cancel()
                return

        # Run base protocols for clientbound
        for base in self._manager.get_base_protocols():
            base.transform_clientbound(packet_id, wrapper, connection)
            if wrapper.cancelled:
                event.cancel()
                return

        new_payload = wrapper.to_bytes()
        if new_payload != payload:
            self._logger.debug(
                f"[CB] {_pname(packet_id)} rewritten "
                f"{len(payload)}b -> {len(new_payload)}b"
            )
            event.payload = new_payload

    def _get_chain(self, connection) -> list[Protocol] | None:
        """Get the protocol chain for a connection, caching the result."""
        if connection.protocol_chain is not None:
            return connection.protocol_chain
        chain = self._manager.get_path(
            connection.server_protocol, connection.client_protocol
        )
        if chain is not None:
            connection.protocol_chain = chain
        return chain
