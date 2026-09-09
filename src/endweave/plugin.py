"""Endweave plugin - protocol translation for Bedrock Edition."""

from pathlib import Path

from endstone.event import (
    EventPriority,
    PacketReceiveEvent,
    PacketSendEvent,
    PlayerJoinEvent,
    event_handler,
)
from endstone.plugin import Plugin

from .commands import CommandHandler
from .config import ConfigurationProvider, EndweaveConfig
from .debug import DebugHandler
from .metrics import EndweaveMetrics
from .update import send_update_message


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

        self.register_events(self)
        self._debug_handler = DebugHandler(
            self.logger, log_conversion_warnings=self._configuration.log_other_conversion_warnings
        )
        self.get_command("endweave").executor = CommandHandler(self._debug_handler)

        # bStats metrics (https://bstats.org/plugin/bukkit/Endweave/30345)
        self._metrics = EndweaveMetrics(self, service_id=30345)

        if self._configuration.check_for_updates:
            send_update_message(self)

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        pass

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_send(self, event: PacketSendEvent) -> None:
        pass

    @event_handler
    def on_player_join(self, event: PlayerJoinEvent) -> None:
        player = event.player
        if player.has_permission("endweave.update") and self._configuration.check_for_updates:
            send_update_message(self, player)
