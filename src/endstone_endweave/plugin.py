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

from endstone_endweave.connection import ConnectionManager
from endstone_endweave.debug import DebugHandler
from endstone_endweave.metrics import EndweaveMetrics
from endstone_endweave.pipeline import ProtocolPipeline
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.base import create_base_protocol
from endstone_endweave.protocol.manager import ProtocolManager
from endstone_endweave.protocol.v859_to_v860 import (
    create_protocol as create_v859_to_v860,
)
from endstone_endweave.protocol.v860_to_v859 import (
    create_protocol as create_v860_to_v859,
)
from endstone_endweave.protocol.v860_to_v898 import (
    create_protocol as create_v860_to_v898,
)
from endstone_endweave.protocol.v898_to_v860 import (
    create_protocol as create_v898_to_v860,
)
from endstone_endweave.protocol.v898_to_v924 import (
    create_protocol as create_v898_to_v924,
)
from endstone_endweave.protocol.v924_to_v898 import (
    create_protocol as create_v924_to_v898,
)
from endstone_endweave.protocol.v924_to_v944 import (
    create_protocol as create_v924_to_v944,
)
from endstone_endweave.protocol.v944_to_v924 import (
    create_protocol as create_v944_to_v924,
)
from endstone_endweave.protocol.versions import VERSIONS


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

        server_protocol = self.server.protocol_version
        self.logger.info(f"Detected server protocol {server_protocol} (MC {self.server.minecraft_version})")
        self._connections = ConnectionManager(server_protocol=server_protocol, logger=self.logger)
        self._manager = ProtocolManager()

        # Register base protocol (version detection + disconnect logging)
        self._manager.register_base(create_base_protocol(server_protocol))

        # Register version-specific protocols
        self._register_protocol(create_v859_to_v860())
        self._register_protocol(create_v860_to_v859())
        self._register_protocol(create_v860_to_v898())
        self._register_protocol(create_v898_to_v860())
        self._register_protocol(create_v898_to_v924())
        self._register_protocol(create_v924_to_v898())
        self._register_protocol(create_v924_to_v944())
        self._register_protocol(create_v944_to_v924())
        # Future: self._register_protocol(create_v944_to_v960())

        self._supported_versions = self._manager.get_supported_versions(server_protocol)
        self._advertised_protocol = max(self._supported_versions) if self._supported_versions else server_protocol

        for protocol in self._supported_versions:
            if protocol not in VERSIONS:
                self.logger.warning(f"Supported protocol {protocol} has no entry in VERSIONS registry")

        supported_names = [
            VERSIONS[protocol].minecraft_version for protocol in self._supported_versions if protocol in VERSIONS
        ]
        if len(supported_names) >= 2:
            self.logger.info(f"Supported client versions: {supported_names[0]} - {supported_names[-1]}")
        elif supported_names:
            self.logger.info(f"Supported client versions: {supported_names[0]}")

        self._pipeline = ProtocolPipeline(self._manager, self._connections, self.logger, debug)
        self.register_events(self)

        # bStats metrics (https://bstats.org/plugin/bukkit/Endweave/30345)
        self._metrics = EndweaveMetrics(self, service_id=30345)

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
        ver = VERSIONS.get(self._advertised_protocol)
        if ver:
            event.minecraft_version_network = ver.minecraft_version

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        self._connections.remove_by_player(event.player)
