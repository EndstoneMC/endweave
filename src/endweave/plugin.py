"""Endweave plugin - protocol translation for Bedrock Edition."""

from endstone.event import (
    EventPriority,
    PacketReceiveEvent,
    PacketSendEvent,
    PlayerJoinEvent,
    event_handler,
)
from endstone.plugin import Plugin

from .commands import CommandHandler
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
        self.save_default_config()

        server_protocol = self.server.protocol_version
        self.logger.info(f"Detected server protocol {server_protocol} (MC {self.server.minecraft_version})")

        self.register_events(self)
        self._debug_handler = DebugHandler(self.logger)
        self.get_command("endweave").executor = CommandHandler(self._debug_handler)

        # bStats metrics (https://bstats.org/plugin/bukkit/Endweave/30345)
        self._metrics = EndweaveMetrics(self, service_id=30345)

        if self.config.get("check-for-updates", True):
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
        if player.has_permission("endweave.update") and self.config.get("check-for-updates", True):
            send_update_message(self, player)
