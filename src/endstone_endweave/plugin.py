"""Endweave plugin - protocol translation for Bedrock Edition."""

from endstone.event import (
    EventPriority,
    PacketReceiveEvent,
    PacketSendEvent,
    PlayerJoinEvent,
    PlayerQuitEvent,
    ServerListPingEvent,
    event_handler,
)
from endstone.plugin import Plugin

from ._version import __version__
from .connection import ConnectionManager
from .debug import DebugHandler
from .metrics import EndweaveMetrics
from .pipeline import ProtocolPipeline
from .protocol import Protocol
from .protocol.base import create_base_protocol
from .protocol.manager import ProtocolManager
from .protocol.v859_to_v860 import create_protocol as create_v859_to_v860
from .protocol.v860_to_v859 import create_protocol as create_v860_to_v859
from .protocol.v860_to_v898 import create_protocol as create_v860_to_v898
from .protocol.v898_to_v860 import create_protocol as create_v898_to_v860
from .protocol.v898_to_v924 import create_protocol as create_v898_to_v924
from .protocol.v924_to_v898 import create_protocol as create_v924_to_v898
from .protocol.v924_to_v944 import create_protocol as create_v924_to_v944
from .protocol.v944_to_v924 import create_protocol as create_v944_to_v924
from .protocol.v944_to_v975 import create_protocol as create_v944_to_v975
from .protocol.versions import VERSIONS
from .update import UpdateChecker


class EndweavePlugin(Plugin):
    """Endstone plugin that enables protocol translation between Bedrock versions.

    Registers event handlers for packet interception and routes packets through
    a ProtocolPipeline that applies version-specific transformations.
    """

    prefix = "Endweave"  # type: ignore[assignment]
    api_version = "0.11"  # type: ignore[assignment]
    permissions = {  # type: ignore[assignment]
        "endweave.update": {
            "description": "Receive update notifications on join",
            "default": "op",
        },
    }

    def on_enable(self) -> None:
        self.save_default_config()
        debug = DebugHandler.from_config(self.logger, self.config)
        if debug.enabled:
            self.logger.set_level(self.logger.DEBUG)

        server_protocol = self.server.protocol_version
        self.logger.info(f"Detected server protocol {server_protocol} (MC {self.server.minecraft_version})")
        self._connections = ConnectionManager(server_protocol=server_protocol, logger=self.logger)
        self._manager = ProtocolManager()

        self._manager.register_base(create_base_protocol(server_protocol))

        for factory in (
            create_v859_to_v860,
            create_v860_to_v859,
            create_v860_to_v898,
            create_v898_to_v860,
            create_v898_to_v924,
            create_v924_to_v898,
            create_v924_to_v944,
            create_v944_to_v924,
            create_v944_to_v975,
        ):
            self._register_protocol(factory())

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

        self._update_checker: UpdateChecker | None = None
        if self.config.get("check-for-updates", True):
            self._update_checker = UpdateChecker(self.logger, __version__)
            self._update_checker.check()

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
    def on_player_join(self, event: PlayerJoinEvent) -> None:
        if self._update_checker:
            self._update_checker.notify_if_needed(event.player)

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        self._connections.remove_by_player(event.player)
