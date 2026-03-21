"""Endweave plugin - protocol translation for Bedrock Edition."""
import platform
from typing import Any, Dict

from endstone.event import (
    EventPriority,
    PacketReceiveEvent,
    PacketSendEvent,
    PlayerQuitEvent,
    ServerListPingEvent,
    event_handler,
)
from endstone.metrics import Metrics
from endstone.plugin import Plugin

from endstone_endweave.connection import ConnectionManager
from endstone_endweave.debug import DebugHandler
from endstone_endweave.pipeline import ProtocolPipeline
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.base import create_base_protocol
from endstone_endweave.protocol.manager import ProtocolManager
from endstone_endweave.protocol.v924_to_v944 import (
    create_protocol as create_v924_to_v944,
)
from endstone_endweave.protocol.versions import VERSIONS, get_version_by_name


class EndweaveMetrics(Metrics):
    """Polyfill for endstone<=0.11.2 Metrics which has runtime errors and
    incorrect data formatting in append_platform_data and append_service_data.
    """

    def append_platform_data(self, platform_data: Dict[str, Any]) -> None:
        super().append_platform_data(platform_data)
        server = self._plugin.server
        if "minecraftVersion" in platform_data:
            platform_data.pop("minecraftVersion")

        if "bukkitVersion" not in platform_data:
            platform_data["bukkitVersion"] = f"{server.version} (MC: {server.minecraft_version})"
            platform_data["bukkitName"] = server.name

        os_arch = platform.machine().lower()
        if os_arch == "x86_64":
            os_arch = "amd64"
        platform_data["osArch"] = os_arch

    def append_service_data(self, service_data: dict[str, object]) -> None:
        description = self._plugin._description
        if description is not None:
            service_data["pluginVersion"] = description.version


class EndweavePlugin(Plugin):
    """Endstone plugin that enables protocol translation between Bedrock versions.

    Registers event handlers for packet interception and routes packets through
    a ProtocolPipeline that applies version-specific transformations.
    """

    prefix = "Endweave"  # type: ignore[assignment]
    api_version = "0.11"  # type: ignore[assignment]

    def on_enable(self) -> None:
        self.save_default_config()
        debug = DebugHandler.from_config(self.logger, self.config)
        if debug.enabled:
            self.logger.set_level(self.logger.DEBUG)

        server_protocol = self._detect_server_protocol()
        self._connections = ConnectionManager(server_protocol=server_protocol, logger=self.logger)
        self._manager = ProtocolManager()

        # Register base protocol (version detection + disconnect logging)
        self._manager.register_base(create_base_protocol(server_protocol))

        # Register version-specific protocols
        self._register_protocol(create_v924_to_v944())
        # Future: self._register_protocol(create_v944_to_v960())

        # Determine highest client version we support
        self._max_client_version = self._manager.get_max_client_version(server_protocol)

        if self._max_client_version:
            server_ver = VERSIONS.get(server_protocol)
            max_ver = VERSIONS.get(self._max_client_version)
            if server_ver and max_ver:
                self.logger.info(
                    f"Supported client versions: {server_ver.minecraft_version}-{max_ver.minecraft_version}"
                )

        self._pipeline = ProtocolPipeline(self._manager, self._connections, self.logger, debug)
        self.register_events(self)

        # bStats metrics (https://bstats.org/plugin/bukkit/Endweave/30345)
        self._metrics = EndweaveMetrics(self, service_id=30345)

    @staticmethod
    def _normalize_mc_version(version: str) -> str:
        """Normalize a Minecraft version string to dotted form.

        Endstone may return short forms like "26.3" meaning "1.26.3".

        Args:
            version: Version string from Endstone (e.g. "26.3" or "1.26.3").

        Returns:
            Dotted version string with major version prefix (e.g. "1.26.3").
        """
        parts = version.split(".")
        if len(parts) == 2:
            # "26.3" -> "1.26.3"
            return f"1.{parts[0]}.{parts[1]}"
        return version

    def _detect_server_protocol(self) -> int:
        """Detect the server's protocol version from its minecraft_version string.

        Returns:
            Protocol version number for the running server. Falls back to
            the lowest known protocol if detection fails.
        """
        server_mc_version = self._normalize_mc_version(self.server.minecraft_version)
        ver = get_version_by_name(server_mc_version)
        if ver:
            self.logger.info(f"Detected server protocol {ver.protocol} (MC {server_mc_version})")
            return ver.protocol
        fallback = min(VERSIONS.keys()) if VERSIONS else 0
        self.logger.warning(f"Could not detect server protocol for MC {server_mc_version}, falling back to {fallback}")
        return fallback

    def _register_protocol(self, protocol: Protocol) -> None:
        self._manager.register(protocol)

    @event_handler(priority=EventPriority.LOWEST)  # type: ignore[func-returns-value,untyped-decorator]
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        self._pipeline.on_packet_receive(event)

    @event_handler(priority=EventPriority.LOWEST)  # type: ignore[func-returns-value,untyped-decorator]
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
        self._connections.remove_by_player(event.player)
