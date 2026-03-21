"""Packet translation pipeline - routes packets through the appropriate protocol.

- Pre/post transform debug logging with packet ID, hex, direction, state
- InformativeException for structured error context
- Active flag on UserConnection for fast-path skip

See Also:
    com.viaversion.viaversion.protocol.ProtocolPipelineImpl
"""

import traceback
from typing import TYPE_CHECKING

from endstone import Logger
from endstone.event import PacketReceiveEvent, PacketSendEvent

from endstone_endweave.codec.wrapper import PacketWrapper
from endstone_endweave.connection import ConnectionManager

if TYPE_CHECKING:
    from endstone_endweave.connection import UserConnection
from endstone_endweave.debug import DebugHandler, packet_label
from endstone_endweave.exception import InformativeException
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.manager import ProtocolManager


class ProtocolPipeline:
    """Intercepts packet events and applies protocol translation.

    Creates a single PacketWrapper and passes it through each protocol's
    transform method in sequence. Logs packets before and after
    transformation when debug is enabled.

    Attributes:
        _manager: ProtocolManager that provides base protocols and version chains.
        _connections: ConnectionManager for per-player state lookup.
        _logger: Endstone logger instance for error output.
        _debug: Debug handler for filtered packet logging.

    See Also:
        com.viaversion.viaversion.protocol.ProtocolPipelineImpl
    """

    def __init__(
        self,
        manager: ProtocolManager,
        connections: ConnectionManager,
        logger: Logger,
        debug: DebugHandler | None = None,
    ) -> None:
        self._manager = manager
        self._connections = connections
        self._logger = logger
        self._debug = debug or DebugHandler(logger)

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

        for base in self._manager.base_protocols:
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

        # PRE transform logging (ViaVersion: logPrePacketTransform)
        if self._debug.log_pre_packet_transform:
            self._debug.log_packet(
                "PRE ",
                address,
                "SERVERBOUND",
                connection.state.value.upper(),
                packet_id,
                connection.client_protocol,
                len(payload),
            )

        # Fresh wrapper from base protocol output for version-specific chain
        wrapper = PacketWrapper(payload, user=connection)

        # Serverbound: apply chain in order (client -> server direction)
        for protocol in chain:
            try:
                protocol.transform(Direction.SERVERBOUND, packet_id, wrapper)
            except Exception as exc:
                err = (
                    InformativeException(exc)
                    .set("Direction", "SERVERBOUND")
                    .set("Packet ID", packet_label(packet_id))
                    .set("Protocol", protocol.name)
                    .set("Address", address)
                    .set("State", connection.state.value.upper())
                )
                if err.should_be_printed:
                    self._logger.error(f"{err.message}\n{traceback.format_exc()}")
                event.cancel()
                return
            if wrapper.cancelled:
                self._debug.log(
                    packet_id,
                    f"Cancelled serverbound {packet_label(packet_id)} for {address}",
                )
                event.cancel()
                return

        new_payload = wrapper.to_bytes()
        if new_payload != payload:
            event.payload = new_payload

        # POST transform logging (ViaVersion: logPostPacketTransform)
        if self._debug.log_post_packet_transform:
            self._debug.log_packet(
                "POST",
                address,
                "SERVERBOUND",
                connection.state.value.upper(),
                packet_id,
                connection.client_protocol,
                len(event.payload),
            )

    def on_packet_send(self, event: PacketSendEvent) -> None:
        """Handle a clientbound (server->client) packet.

        Args:
            event: Endstone packet send event with readable/writable payload.
        """
        address = str(event.address)

        connection = self._connections.get(address)
        if connection is None or not connection.active:
            return  # fast path (ViaVersion: !connection.isActive())

        chain = self._get_chain(connection)
        if chain is None:
            return

        packet_id = event.packet_id
        payload = event.payload

        # PRE transform logging
        if self._debug.log_pre_packet_transform:
            self._debug.log_packet(
                "PRE ",
                address,
                "CLIENTBOUND",
                connection.state.value.upper(),
                packet_id,
                connection.client_protocol,
                len(payload),
            )

        wrapper = PacketWrapper(payload, user=connection)

        # Clientbound: apply chain in reverse order (server -> client direction)
        for protocol in reversed(chain):
            try:
                protocol.transform(Direction.CLIENTBOUND, packet_id, wrapper)
            except Exception as exc:
                err = (
                    InformativeException(exc)
                    .set("Direction", "CLIENTBOUND")
                    .set("Packet ID", packet_label(packet_id))
                    .set("Protocol", protocol.name)
                    .set("Address", address)
                    .set("State", connection.state.value.upper())
                )
                if err.should_be_printed:
                    self._logger.error(f"{err.message}\n{traceback.format_exc()}")
                event.cancel()
                return
            if wrapper.cancelled:
                self._debug.log(
                    packet_id,
                    f"Cancelled clientbound {packet_label(packet_id)} for {address}",
                )
                event.cancel()
                return

        # Finalize version chain output, then run base protocols with fresh wrapper
        payload = wrapper.to_bytes()
        wrapper = PacketWrapper(payload, user=connection)

        for base in self._manager.base_protocols:
            base.transform(Direction.CLIENTBOUND, packet_id, wrapper)
            if wrapper.cancelled:
                event.cancel()
                return

        new_payload = wrapper.to_bytes()
        if new_payload != event.payload:
            event.payload = new_payload

        # POST transform logging
        if self._debug.log_post_packet_transform:
            self._debug.log_packet(
                "POST",
                address,
                "CLIENTBOUND",
                connection.state.value.upper(),
                packet_id,
                connection.client_protocol,
                len(event.payload),
            )

    def _get_chain(self, connection: "UserConnection") -> list[Protocol] | None:
        """Get the protocol chain for a connection, caching the result.

        Calls Protocol.init() on each protocol when the chain is first resolved.

        Args:
            connection: UserConnection whose server/client protocols determine the chain.

        Returns:
            Ordered list of Protocol instances forming the translation chain,
            or None if no path exists between the server and client versions.

        See Also:
            com.viaversion.viaversion.protocol.ProtocolPipelineImpl#add
        """
        if connection.protocol_chain is not None:
            return connection.protocol_chain
        chain = self._manager.get_path(connection.server_protocol, connection.client_protocol)
        if chain is not None:
            for protocol in chain:
                protocol.init(connection)
            connection.protocol_chain = chain
            connection.active = True  # ViaVersion: setActive(true) after pipeline setup
        return chain
