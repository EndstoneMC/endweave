"""Registry mapping protocol version pairs to translators."""

from __future__ import annotations

from endstone_endweave.player_state import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation, ProtocolTranslator


class TranslatorRegistry:
    """Maps (server_protocol, client_protocol) -> ProtocolTranslator."""

    def __init__(self) -> None:
        self._translators: dict[tuple[int, int], ProtocolTranslator] = {}

    def register(self, translator: ProtocolTranslator) -> None:
        key = (translator.server_protocol, translator.client_protocol)
        self._translators[key] = translator

    def get(self, server_protocol: int, client_protocol: int) -> ProtocolTranslator | None:
        return self._translators.get((server_protocol, client_protocol))
