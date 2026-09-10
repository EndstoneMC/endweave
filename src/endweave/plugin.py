"""Endweave plugin - protocol translation for Bedrock Edition.

Endstone's events stand in for the netty pipeline ViaVersion installs itself
into. A client states its protocol version in RequestNetworkSettings, the first
packet of a Bedrock connection, where ViaVersion reads it off the Java
handshake. That is also where the connection's two translators are resolved, so
every packet after it is carried by the packet events: received packets towards
the server's version, sent packets towards the client's. The blocked version
gate then runs on the login event, the last point before the world is streamed,
in place of the disconnect packet ViaVersion writes into the pipe itself. The
connection is tracked from there until the player quits.

See Also:
    com.viaversion.viaversion.protocols.base.v1_7.ServerboundBaseProtocol1_7
"""

from pathlib import Path

from endstone.event import (
    EventPriority,
    PacketReceiveEvent,
    PacketSendEvent,
    PlayerJoinEvent,
    PlayerLoginEvent,
    PlayerQuitEvent,
    event_handler,
)
from endstone.plugin import Plugin

from ._pipeline import Action, TranslationError, packet_name
from .commands import CommandHandler
from .config import ConfigurationProvider, EndweaveConfig
from .connection import Connection, ConnectionManager
from .debug import DebugHandler, Direction, Packet, PacketType
from .metrics import EndweaveMetrics
from .protocol.version import UNKNOWN, get_by_name, get_protocol
from .update import send_update_message
from .util import translate_alternate_color_codes

_REQUEST_NETWORK_SETTINGS = 193
_TRANSLATION_FAILED = "§cEndweave could not translate a packet for your version."


class EndweavePlugin(Plugin):
    """Endstone plugin that enables protocol translation between Bedrock versions."""

    prefix = "Endweave"
    api_version = "0.11"
    commands = {
        "endweave": {
            "description": "Endweave plugin commands",
            "usages": [
                "/endweave",
                "/endweave list",
                "/endweave debug [clear|pre|post]",
                "/endweave debug <add|remove> <packet: string>",
                "/endweave reload",
            ],
            "permissions": ["endweave.command"],
        }
    }
    permissions = {
        "endweave.admin": {
            "default": "op",
            "children": {
                "endweave.command": True,
                "endweave.command.list": True,
                "endweave.command.debug": True,
                "endweave.command.reload": True,
            },
        },
    }

    def on_enable(self) -> None:
        self._configuration = EndweaveConfig(Path(self.data_folder) / "config.toml", self.logger)
        self._configuration_provider = ConfigurationProvider()
        self._configuration_provider.register(self._configuration)
        self._configuration_provider.reload_configs()

        server_protocol = self.server.protocol_version
        self.logger.info(f"Detected server protocol {server_protocol} (MC {self.server.minecraft_version})")
        self._connection_manager = ConnectionManager(get_protocol(server_protocol))

        self.register_events(self)
        self._debug_handler = DebugHandler(
            self.logger, log_conversion_warnings=self._configuration.log_other_conversion_warnings
        )
        self.get_command("endweave").executor = CommandHandler(self._debug_handler, self._configuration_provider)

        # bStats metrics (https://bstats.org/plugin/bukkit/Endweave/30345)
        self._metrics = EndweaveMetrics(self, service_id=30345)

        if self._configuration.check_for_updates:
            send_update_message(self)

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        if event.packet_id == _REQUEST_NETWORK_SETTINGS:
            payload = event.payload
            if len(payload) < 4:
                return

            client_network_version = int.from_bytes(payload[:4], "big", signed=True)
            protocol_version = get_protocol(client_network_version)
            connection = self._connection_manager.get_or_create(str(event.address))
            connection.protocol_version = protocol_version
            return

        connection = self._connection_manager.get_connection(str(event.address))
        if connection is not None:
            self._translate(connection, event, Direction.SERVERBOUND)

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_send(self, event: PacketSendEvent) -> None:
        connection = self._connection_manager.get_connection(str(event.address))
        if connection is not None:
            self._translate(connection, event, Direction.CLIENTBOUND)

    def _translate(
        self,
        connection: Connection,
        event: PacketReceiveEvent | PacketSendEvent,
        direction: Direction,
    ) -> None:
        """Carry one packet across, where the connection has a translator that names its ID.

        Args:
            connection: The peer the packet belongs to.
            event: The packet event, whose payload is replaced in place.
            direction: Direction the packet travels in.
        """
        translator = connection.serverbound if direction is Direction.SERVERBOUND else connection.clientbound
        if translator is None:
            return

        packet_id = event.packet_id
        action = translator.actions.get(packet_id)
        if action is None:
            return

        debug = self._debug_handler
        label: str | int = packet_id
        logged = False
        if debug.enabled:
            name = packet_name(packet_id)
            packet_type = PacketType(packet_id, name.upper(), direction) if name is not None else None
            logged = debug.should_log(Packet(packet_id, packet_type), direction)
            if packet_type is not None:
                label = packet_type.name

        if action is Action.CANCEL:
            if logged:
                self.logger.info(f"[{direction.value}] {label} dropped, the other side has no such packet")
            event.cancel()
            return

        if logged and debug.log_pre_packet_transform:
            self.logger.info(f"[{direction.value}] {label} in: {event.payload.hex()}")

        try:
            payload = translator.translate(connection.session, packet_id, event.payload)
        except TranslationError as error:
            debug.error(f"Failed to translate {direction.value} packet {packet_name(packet_id) or packet_id}", error)
            event.cancel()
            connection.disconnect(_TRANSLATION_FAILED)
            return

        if payload is None:
            if logged:
                self.logger.info(f"[{direction.value}] {label} refused by a transform")
            event.cancel()
            return

        if logged and debug.log_post_packet_transform:
            self.logger.info(f"[{direction.value}] {label} out: {payload.hex()}")

        event.payload = payload

    @event_handler
    def on_player_login(self, event: PlayerLoginEvent) -> None:
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
                self.logger.info(
                    f"Blocked join due to unsupported version from {player.address} ({protocol_version.name})"
                )
            if connection is not None:
                self._connection_manager.on_disconnect(connection)
            return

        if connection is not None:
            connection.player = player

    @event_handler
    def on_player_join(self, event: PlayerJoinEvent) -> None:
        player = event.player
        if player.has_permission("endweave.update") and self._configuration.check_for_updates:
            send_update_message(self, player)

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        player = event.player
        connection = self._connection_manager.get_connection(str(player.address))
        if connection is None or connection.player is None or connection.player.unique_id != player.unique_id:
            return

        self._connection_manager.on_disconnect(connection)
