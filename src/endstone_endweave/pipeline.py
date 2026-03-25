"""Packet translation pipeline - routes packets through the appropriate protocol.

Base protocols and version-specific protocols are merged into a single list
per connection (ViaVersion: ProtocolPipelineImpl). Serverbound iterates the
list in order (base first, then chain); clientbound iterates with base first
and version chain reversed.

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

    Builds a per-connection protocol list (base + version chain) and
    iterates it in a single pass per packet, matching ViaVersion's design.

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

        connection = self._connections.get_or_create(address)
        pipeline = self._get_pipeline(connection)

        # PRE transform logging (ViaVersion: logPrePacketTransform)
        if connection.needs_translation and self._debug.log_pre_packet_transform:
            self._debug.log_packet(
                "PRE ",
                address,
                "SERVERBOUND",
                packet_id,
                connection.client_protocol,
                len(payload),
            )

        # Serverbound: [base, chain...] in order.
        # Each protocol gets a fresh wrapper from the previous protocol's output.
        for protocol in pipeline:
            wrapper = PacketWrapper(payload, user=connection)
            try:
                protocol.transform(Direction.SERVERBOUND, packet_id, wrapper)
            except Exception as exc:
                err = (
                    InformativeException(exc)
                    .set("Direction", "SERVERBOUND")
                    .set("Packet ID", packet_label(packet_id))
                    .set("Protocol", protocol.name)
                    .set("Address", address)
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
            payload = wrapper.to_bytes()

        if payload != event.payload:
            event.payload = payload

        # POST transform logging (ViaVersion: logPostPacketTransform)
        if connection.needs_translation and self._debug.log_post_packet_transform:
            self._debug.log_packet(
                "POST",
                address,
                "SERVERBOUND",
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
        if connection is None or connection.protocol_pipeline is None:
            return  # pre-handshake: no pipeline yet

        packet_id = event.packet_id
        payload = event.payload

        # PRE transform logging
        if connection.needs_translation and self._debug.log_pre_packet_transform:
            self._debug.log_packet(
                "PRE ",
                address,
                "CLIENTBOUND",
                packet_id,
                connection.client_protocol,
                len(payload),
            )

        # Clientbound: [base, ...reversed chain] (ViaVersion: reversedProtocolList).
        # Each protocol gets a fresh wrapper from the previous protocol's output.
        for protocol in self._clientbound_order(connection):
            wrapper = PacketWrapper(payload, user=connection)
            try:
                protocol.transform(Direction.CLIENTBOUND, packet_id, wrapper)
            except Exception as exc:
                err = (
                    InformativeException(exc)
                    .set("Direction", "CLIENTBOUND")
                    .set("Packet ID", packet_label(packet_id))
                    .set("Protocol", protocol.name)
                    .set("Address", address)
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
            payload = wrapper.to_bytes()

        if payload != event.payload:
            event.payload = payload

        # POST transform logging
        if connection.needs_translation and self._debug.log_post_packet_transform:
            self._debug.log_packet(
                "POST",
                address,
                "CLIENTBOUND",
                packet_id,
                connection.client_protocol,
                len(event.payload),
            )

    def _get_pipeline(self, connection: "UserConnection") -> list[Protocol]:
        """Build or return the cached protocol pipeline for a connection.

        The pipeline is a flat list: [base protocols, version chain...].
        Base protocols are always included. The version chain is appended
        once the client protocol is known and a path exists.

        Args:
            connection: UserConnection whose server/client protocols determine the chain.

        Returns:
            Ordered list of Protocol instances (base + chain).

        See Also:
            com.viaversion.viaversion.protocol.ProtocolPipelineImpl#add
        """
        if connection.protocol_pipeline is not None:
            return connection.protocol_pipeline

        base = list(self._manager.base_protocols)

        if connection.client_protocol == 0:
            # Not yet detected (pre-handshake): return base-only without caching
            return base

        if not connection.needs_translation:
            # Same version: cache base-only pipeline
            connection.protocol_pipeline = base
            return base

        chain = self._manager.get_path(connection.server_protocol, connection.client_protocol)
        if chain is None:
            if not connection.warned_no_chain:
                connection.warned_no_chain = True
                self._logger.warning(
                    f"No protocol chain for server={connection.server_protocol} "
                    f"client={connection.client_protocol} from {connection.address}"
                )
            connection.protocol_pipeline = base
            return base

        for protocol in chain:
            protocol.init(connection)

        pipeline = base + chain
        connection.protocol_pipeline = pipeline
        return pipeline

    @staticmethod
    def _clientbound_order(connection: "UserConnection") -> list[Protocol]:
        """Return the clientbound iteration order: base first, then chain reversed.

        Mirrors ViaVersion's ``refreshReversedList()``: base protocols in
        regular order, followed by non-base protocols in reverse order.

        See Also:
            com.viaversion.viaversion.protocol.ProtocolPipelineImpl#refreshReversedList
        """
        pipeline = connection.protocol_pipeline
        if pipeline is None:
            return []

        # Find where base protocols end and version chain begins.
        # Base protocols have name="base" (convention from create_base_protocol).
        base = [p for p in pipeline if p.is_base]
        chain = [p for p in pipeline if not p.is_base]
        return base + list(reversed(chain))
