"""Endweave plugin - protocol translation for Bedrock Edition."""

from endstone.event import (
    EventPriority,
    PacketReceiveEvent,
    PacketSendEvent,
    PlayerQuitEvent,
    ServerListPingEvent,
    event_handler,
)
from endstone.plugin import Plugin

from endstone_endweave.pipeline import TranslationPipeline
from endstone_endweave.player_state import SessionManager
from endstone_endweave.protocol.registry import TranslatorRegistry
from endstone_endweave.protocol.v924_to_v944 import create_v924_to_v944


class EndweavePlugin(Plugin):
    prefix = "Endweave"
    api_version = "0.11"

    def on_enable(self) -> None:
        self._sessions = SessionManager()
        self._registry = TranslatorRegistry()
        self._registry.register(create_v924_to_v944())
        self._pipeline = TranslationPipeline(
            self._registry, self._sessions, self.logger
        )
        self.register_events(self)
        self.logger.info(
            "Endweave enabled - translating proto 944 clients -> 924 server"
        )

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        self._pipeline.on_packet_receive(event)

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_send(self, event: PacketSendEvent) -> None:
        self._pipeline.on_packet_send(event)

    @event_handler
    def on_server_list_ping(self, event: ServerListPingEvent) -> None:
        # Advertise the newer version so v944 clients see a compatible server
        event.minecraft_version_network = "1.26.10"

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        self._sessions.remove_by_player(event.player)
