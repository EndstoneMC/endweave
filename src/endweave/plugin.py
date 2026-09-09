"""Endweave plugin - protocol translation for Bedrock Edition.

Endstone's events stand in for the netty pipeline ViaVersion installs itself
into. A client states its protocol version in RequestNetworkSettings, the first
packet of a Bedrock connection, where ViaVersion reads it off the Java
handshake. The blocked version gate then runs on the login event, the last
point before the world is streamed, in place of the disconnect packet
ViaVersion writes into the pipe itself. The connection is tracked from there
until the player quits.

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

from .commands import CommandHandler
from .config import ConfigurationProvider, EndweaveConfig
from .connection import ConnectionManager
from .debug import DebugHandler
from .metrics import EndweaveMetrics
from .protocol.version import get_protocol
from .update import send_update_message
from .util import translate_alternate_color_codes

_REQUEST_NETWORK_SETTINGS = 193


class EndweavePlugin(Plugin):
    """Endstone plugin that enables protocol translation between Bedrock versions."""

    prefix = "Endweave"
    api_version = "0.11"
    commands = {
        "endweave": {
            "description": "Endweave plugin commands",
            "usages": [
                "/endweave list",
                "/endweave debug [clear|pre|post]",
                "/endweave debug <add|remove> <packet: string>",
                "/endweave reload",
            ],
            "permission": "endweave.command",
        }
    }
    permissions = {
        "endweave.admin": {
            "default": "op",
            "children": {
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
        if event.packet_id != _REQUEST_NETWORK_SETTINGS:
            return

        payload = event.payload
        if len(payload) < 4:
            return

        client_network_version = int.from_bytes(payload[:4], "big", signed=True)
        connection = self._connection_manager.get_or_create(str(event.address))
        connection.protocol_version = get_protocol(client_network_version)

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_send(self, event: PacketSendEvent) -> None:
        pass

    @event_handler
    def on_player_login(self, event: PlayerLoginEvent) -> None:
        player = event.player
        connection = self._connection_manager.get_connection(str(player.address))
        if connection is None:
            return

        protocol_version = connection.protocol_version
        if protocol_version in self._configuration.blocked_protocol_versions:
            event.kick_message = translate_alternate_color_codes(self._configuration.blocked_disconnect_message)
            event.cancel()
            if self._configuration.log_blocked_joins:
                self.logger.info(
                    f"Blocked join due to unsupported version from {player.address} ({protocol_version.name})"
                )
            self._connection_manager.on_disconnect(connection)
            return

        connection.player = player

    @event_handler
    def on_player_join(self, event: PlayerJoinEvent) -> None:
        player = event.player
        if player.has_permission("endweave.update") and self._configuration.check_for_updates:
            send_update_message(self, player)

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        connection = self._connection_manager.get_connection(str(event.player.address))
        if connection is not None:
            self._connection_manager.on_disconnect(connection)
