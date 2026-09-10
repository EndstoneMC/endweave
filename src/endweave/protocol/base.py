"""The base protocol, which every connection runs through ahead of its translators.

A client states its protocol version in RequestNetworkSettings, the first
packet of a Bedrock connection, where ViaVersion's InitialBaseProtocol reads it
off the Java handshake. The pipeline is installed there, and the server's own
version written over the client's, as InitialBaseProtocol does. Login states the
version a second time, which BDS checks as well, so it is rewritten too. A
connection without translators is left as it is: where the engine reads both
ends as one version, or does not carry one of them, the server takes or refuses
the client itself. Translators are shared by every connection between the same
two versions, as ViaVersion shares its protocols.

The blocked version gate of ServerboundBaseProtocol1_7 and the login bookkeeping
of ClientboundBaseProtocol1_7 run on the login event, the last point before the
world is streamed, in place of the disconnect packet ViaVersion writes into the
pipe itself. Left out are the connection state all three track, which Bedrock
has no counterpart for, the status response, since the server list ping never
reaches a packet event, and the compression threshold, which Endstone handles
below this.

See Also:
    com.viaversion.viaversion.protocols.base.InitialBaseProtocol
    com.viaversion.viaversion.protocols.base.v1_7.ClientboundBaseProtocol1_7
    com.viaversion.viaversion.protocols.base.v1_7.ServerboundBaseProtocol1_7
"""

from __future__ import annotations

import functools

from endstone import Logger
from endstone.event import PacketReceiveEvent, PlayerLoginEvent

from .. import _pipeline
from ..config import EndweaveConfig
from ..connection import Connection, ConnectionManager
from ..util import translate_alternate_color_codes
from .version import UNKNOWN, get_by_name, get_protocol

__all__ = ["BaseProtocol"]

_REQUEST_NETWORK_SETTINGS = 193
_LOGIN = 1

_translator = functools.cache(_pipeline.Translator)


class BaseProtocol:
    """The protocol at the head of every connection's pipeline.

    See Also:
        com.viaversion.viaversion.protocols.base.InitialBaseProtocol
        com.viaversion.viaversion.protocols.base.v1_7.ClientboundBaseProtocol1_7
        com.viaversion.viaversion.protocols.base.v1_7.ServerboundBaseProtocol1_7
    """

    def __init__(self, connection_manager: ConnectionManager, configuration: EndweaveConfig, logger: Logger) -> None:
        self._connection_manager = connection_manager
        self._configuration = configuration
        self._logger = logger

    def transform_serverbound(self, event: PacketReceiveEvent) -> Connection | None:
        address = str(event.address)
        packet_id = event.packet_id
        if packet_id != _REQUEST_NETWORK_SETTINGS:
            connection = self._connection_manager.get_connection(address)
            if packet_id != _LOGIN or connection is None or connection.serverbound is None:
                return connection

            payload = event.payload
            if len(payload) >= 4:
                server = connection.server_protocol_version.version
                event.payload = server.to_bytes(4, "big", signed=True) + payload[4:]
            return connection

        payload = event.payload
        hostname = address.rpartition(":")[0]
        if len(payload) < 4 or not hostname:
            return None

        protocol_version = get_protocol(int.from_bytes(payload[:4], "big", signed=True))
        connection = self._connection_manager.get_or_create(address)
        connection.protocol_version = protocol_version
        connection.serverbound = None
        connection.clientbound = None
        client = protocol_version.version
        server = connection.server_protocol_version.version
        try:
            serverbound = _translator(client, server)
        except ValueError:
            return connection

        if serverbound.from_version != serverbound.to_version:
            connection.serverbound = serverbound
            connection.clientbound = _translator(server, client)
            event.payload = server.to_bytes(4, "big", signed=True) + payload[4:]
        return connection

    def on_login(self, event: PlayerLoginEvent) -> None:
        player = event.player
        connection = self._connection_manager.get_connection(str(player.address))
        if event.is_cancelled:
            if connection is not None:
                self._connection_manager.on_disconnect(connection)
            return

        protocol_version = connection.protocol_version if connection is not None else UNKNOWN
        if protocol_version == UNKNOWN:
            protocol_version = get_by_name(player.game_version) or UNKNOWN

        if protocol_version in self._configuration.blocked_protocol_versions:
            event.kick_message = translate_alternate_color_codes(self._configuration.blocked_disconnect_message)
            event.cancel()
            if self._configuration.log_blocked_joins:
                self._logger.info(
                    f"Blocked join due to unsupported version from {player.address} ({protocol_version.name})"
                )
            if connection is not None:
                self._connection_manager.on_disconnect(connection)
            return

        if connection is not None:
            connection.player = player
