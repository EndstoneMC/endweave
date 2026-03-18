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
from endstone_endweave.protocol.versions import VERSIONS, get_version_by_name


class EndweavePlugin(Plugin):
    prefix = "Endweave"  # type: ignore[assignment]
    api_version = "0.11"  # type: ignore[assignment]

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

    @staticmethod
    def _normalize_mc_version(version: str) -> str:
        """Normalize a Minecraft version string to dotted form.

        Endstone may return short forms like "26.3" meaning "1.26.3".
        """
        parts = version.split(".")
        if len(parts) == 2:
            # "26.3" -> "1.26.3"
            return f"1.{parts[0]}.{parts[1]}"
        return version

    def _detect_server_protocol(self) -> int:
        """Detect the server's protocol version from its minecraft_version string."""
        server_mc_version = self._normalize_mc_version(self.server.minecraft_version)
        ver = get_version_by_name(server_mc_version)
        if ver:
            self.logger.info(f"Detected server protocol {ver.protocol} (MC {server_mc_version})")
            return ver.protocol
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

    @event_handler(priority=EventPriority.LOWEST)  # type: ignore[func-returns-value]
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        self._pipeline.on_packet_receive(event)

    @event_handler(priority=EventPriority.LOWEST)  # type: ignore[func-returns-value]
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
