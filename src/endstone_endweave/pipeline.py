"""Packet translation pipeline - routes packets through the appropriate protocol."""

import traceback

from endstone import Logger
from endstone.event import PacketReceiveEvent, PacketSendEvent

from endstone_endweave.codec.wrapper import PacketWrapper
from endstone_endweave.connection import ConnectionManager
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.manager import ProtocolManager


def _pname(packet_id: int) -> str:
    """Resolve packet ID to name, e.g. 'START_GAME(11)' or '999'.

    Args:
        packet_id: Numeric Bedrock packet identifier.

    Returns:
        Human-readable string like 'START_GAME(11)', or the raw number
        as a string if the ID is not in the PacketId enum.
    """
    try:
        return f"{PacketId(packet_id).name}({packet_id})"
    except ValueError:
        return str(packet_id)


class ProtocolPipeline:
    """Intercepts packet events and applies protocol translation.

    Like ViaVersion's ProtocolPipelineImpl, creates a single PacketWrapper
    and passes it through each protocol's transform method in sequence.

    Attributes:
        _manager: ProtocolManager that provides base protocols and version chains.
        _connections: ConnectionManager for per-player state lookup.
        _logger: Endstone logger instance for debug and error output.
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
        """Handle a serverbound (client->server) packet.

        Args:
            event: Endstone packet receive event with readable/writable payload.
        """
        address = str(event.address)
        packet_id = event.packet_id
        payload = event.payload

        # Run base protocols first (always, even before needs_translation check)
        connection = self._connections.get_or_create(address)
        wrapper = PacketWrapper(payload, user=connection)

        for base in self._manager.get_base_protocols():
            base.transform(Direction.SERVERBOUND, packet_id, wrapper)
            if wrapper.cancelled:
                event.cancel()
                return

        # Finalize base protocol output before version-specific chain
        payload = wrapper.to_bytes()

        if not connection.needs_translation:
            if payload != event.payload:
                event.payload = payload
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
            if payload != event.payload:
                event.payload = payload
            return

        # Fresh wrapper from base protocol output for version-specific chain
        wrapper = PacketWrapper(payload, user=connection)

        # Serverbound: apply chain in order (client -> server direction)
        for protocol in chain:
            try:
                protocol.transform(Direction.SERVERBOUND, packet_id, wrapper)
            except Exception:
                self._logger.error(
                    f"[SB] {_pname(packet_id)} from {address} "
                    f"EXCEPTION:\n{traceback.format_exc()}"
                )
                event.cancel()
                return
            if wrapper.cancelled:
                self._logger.debug(f"[SB] {_pname(packet_id)} CANCELLED")
                event.cancel()
                return

        new_payload = wrapper.to_bytes()
        if new_payload != payload:
            self._logger.debug(
                f"[SB] {_pname(packet_id)} rewritten "
                f"{len(event.payload)}b -> {len(new_payload)}b"
            )
            event.payload = new_payload

    def on_packet_send(self, event: PacketSendEvent) -> None:
        """Handle a clientbound (server->client) packet.

        Args:
            event: Endstone packet send event with readable/writable payload.
        """
        address = str(event.address)

        connection = self._connections.get(address)
        if connection is None or not connection.needs_translation:
            return  # fast path

        chain = self._get_chain(connection)
        if chain is None:
            return

        packet_id = event.packet_id
        payload = event.payload
        wrapper = PacketWrapper(payload, user=connection)

        self._logger.debug(f"[CB] {_pname(packet_id)} ({len(payload)}b)")

        # Clientbound: apply chain in reverse order (server -> client direction)
        for protocol in reversed(chain):
            try:
                protocol.transform(Direction.CLIENTBOUND, packet_id, wrapper)
            except Exception:
                self._logger.error(
                    f"[CB] {_pname(packet_id)} to {address} "
                    f"EXCEPTION:\n{traceback.format_exc()}"
                )
                event.cancel()
                return
            if wrapper.cancelled:
                self._logger.debug(f"[CB] {_pname(packet_id)} CANCELLED")
                event.cancel()
                return

        # Finalize version chain output, then run base protocols with fresh wrapper
        payload = wrapper.to_bytes()
        wrapper = PacketWrapper(payload, user=connection)

        for base in self._manager.get_base_protocols():
            base.transform(Direction.CLIENTBOUND, packet_id, wrapper)
            if wrapper.cancelled:
                event.cancel()
                return

        new_payload = wrapper.to_bytes()
        if new_payload != event.payload:
            self._logger.debug(
                f"[CB] {_pname(packet_id)} rewritten "
                f"{len(event.payload)}b -> {len(new_payload)}b"
            )
            event.payload = new_payload

    def _get_chain(self, connection) -> list[Protocol] | None:
        """Get the protocol chain for a connection, caching the result.

        Args:
            connection: UserConnection whose server/client protocols determine the chain.

        Returns:
            Ordered list of Protocol instances forming the translation chain,
            or None if no path exists between the server and client versions.
        """
        if connection.protocol_chain is not None:
            return connection.protocol_chain
        chain = self._manager.get_path(
            connection.server_protocol, connection.client_protocol
        )
        if chain is not None:
            connection.protocol_chain = chain
        return chain
