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
from endstone_endweave.session import SessionManager
from endstone_endweave.protocol.base import ProtocolTranslator
from endstone_endweave.protocol.registry import TranslatorRegistry
from endstone_endweave.protocol.v924_to_v944 import create_translator as create_v924_to_v944
from endstone_endweave.protocol.versions import VERSIONS


class EndweavePlugin(Plugin):
    prefix = "Endweave"
    api_version = "0.11"

    def on_enable(self) -> None:
        server_protocol = self._detect_server_protocol()
        self._sessions = SessionManager(server_protocol=server_protocol)
        self._registry = TranslatorRegistry()

        # Register translators (add new ones here, like ViaVersion's registerProtocols)
        self._register_translator(create_v924_to_v944())
        # Future: self._register_translator(create_v944_to_v960())

        # Determine highest client version we support
        self._max_client_version = self._registry.get_max_client_version(server_protocol)

        self._pipeline = TranslationPipeline(
            self._registry, self._sessions, self.logger
        )
        self.register_events(self)

        max_ver = VERSIONS.get(self._max_client_version) if self._max_client_version else None
        self.logger.info(
            f"Endweave enabled - server proto {server_protocol}, "
            f"max client: {max_ver.minecraft_version if max_ver else 'none'}"
        )

    def _detect_server_protocol(self) -> int:
        """Detect the server's protocol version from its minecraft_version string."""
        server_mc_version = self.server.minecraft_version
        for proto, ver in VERSIONS.items():
            if ver.minecraft_version == server_mc_version:
                self.logger.info(
                    f"Detected server protocol {proto} "
                    f"(MC {server_mc_version})"
                )
                return proto
        # Fallback: assume lowest registered version
        fallback = min(VERSIONS.keys()) if VERSIONS else 0
        self.logger.warning(
            f"Could not detect server protocol for MC {server_mc_version}, "
            f"falling back to {fallback}"
        )
        return fallback

    def _register_translator(self, translator: ProtocolTranslator) -> None:
        self._registry.register(translator)
        self.logger.info(
            f"Registered translator: "
            f"{translator.server_protocol} -> {translator.client_protocol}"
        )

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        self._pipeline.on_packet_receive(event)

    @event_handler(priority=EventPriority.LOWEST)
    def on_packet_send(self, event: PacketSendEvent) -> None:
        self._pipeline.on_packet_send(event)

    @event_handler
    def on_server_list_ping(self, event: ServerListPingEvent) -> None:
        if self._max_client_version:
            ver = VERSIONS.get(self._max_client_version)
            if ver:
                event.minecraft_version_network = ver.minecraft_version

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        self._sessions.remove_by_player(event.player)
