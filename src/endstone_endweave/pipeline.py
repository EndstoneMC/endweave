"""Packet translation pipeline - routes packets through the appropriate translator."""

from __future__ import annotations

import struct
import traceback

from endstone import Logger
from endstone.event import PacketReceiveEvent, PacketSendEvent

from endstone_endweave.codec import PacketReader
from endstone_endweave.session import SessionManager
from endstone_endweave.protocol.base import PacketTransformation, ProtocolTranslator
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.registry import TranslatorRegistry


def _pname(packet_id: int) -> str:
    """Resolve packet ID to name, e.g. 'START_GAME(11)' or '999'."""
    try:
        return f"{PacketId(packet_id).name}({packet_id})"
    except ValueError:
        return str(packet_id)


class TranslationPipeline:
    """Intercepts packet events and applies protocol translation."""

    def __init__(
        self,
        registry: TranslatorRegistry,
        sessions: SessionManager,
        logger: Logger,
    ) -> None:
        self._registry = registry
        self._sessions = sessions
        self._logger = logger

    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        """Handle a serverbound (client->server) packet."""
        address = str(event.address)
        packet_id = event.packet_id
        payload = event.payload

        # Detect client protocol from RequestNetworkSettings
        if packet_id == PacketId.REQUEST_NETWORK_SETTINGS:
            new_session = self._sessions.get_or_create(address)
            if len(payload) >= 4:
                client_proto = struct.unpack(">i", payload[:4])[0]
                new_session.client_protocol = client_proto
                self._logger.info(
                    f"Client {address} connecting with protocol {client_proto}"
                )

        session = self._sessions.get(address)
        if session is None or not session.needs_translation:
            return  # fast path

        self._logger.info(f"[SB] {_pname(packet_id)} ({len(payload)}b)")

        chain = self._get_chain(session)
        if chain is None:
            if not session.warned_no_translator:
                session.warned_no_translator = True
                self._logger.warning(
                    f"No translator for server={session.server_protocol} "
                    f"client={session.client_protocol} from {address}"
                )
            return

        # Serverbound: apply chain in order (client -> server direction)
        for translator in chain:
            try:
                result = translator.translate_serverbound(packet_id, payload, session)
            except Exception:
                self._logger.error(
                    f"[SB] {_pname(packet_id)} ({len(payload)}b) from {address} "
                    f"EXCEPTION:\n{traceback.format_exc()}"
                )
                return
            if result.cancel:
                self._logger.info(f"[SB] {_pname(packet_id)} CANCELLED")
                event.cancel()
                return
            if result.new_payload is not None:
                self._logger.info(
                    f"[SB] {_pname(packet_id)} rewritten "
                    f"{len(payload)}b -> {len(result.new_payload)}b"
                )
                payload = result.new_payload
                event.payload = payload

    def on_packet_send(self, event: PacketSendEvent) -> None:
        """Handle a clientbound (server->client) packet."""
        address = str(event.address)

        session = self._sessions.get(address)
        if session is None or not session.needs_translation:
            return  # fast path

        chain = self._get_chain(session)
        if chain is None:
            return

        packet_id = event.packet_id
        payload = event.payload

        self._logger.info(f"[CB] {_pname(packet_id)} ({len(payload)}b)")

        if packet_id == PacketId.DISCONNECT:
            self._log_disconnect(address, payload)

        # Clientbound: apply chain in reverse order (server -> client direction)
        for translator in reversed(chain):
            try:
                result = translator.translate_clientbound(packet_id, payload, session)
            except Exception:
                self._logger.error(
                    f"[CB] {_pname(packet_id)} ({len(payload)}b) to {address} "
                    f"EXCEPTION:\n{traceback.format_exc()}"
                )
                return
            if result.cancel:
                self._logger.info(f"[CB] {_pname(packet_id)} CANCELLED")
                event.cancel()
                return
            if result.new_payload is not None:
                self._logger.info(
                    f"[CB] {_pname(packet_id)} rewritten "
                    f"{len(payload)}b -> {len(result.new_payload)}b"
                )
                payload = result.new_payload
                event.payload = payload

    def _get_chain(self, session) -> list[ProtocolTranslator] | None:
        """Get the translator chain for a session, caching the result."""
        if session.translator_chain is not None:
            return session.translator_chain
        chain = self._registry.get_path(
            session.server_protocol, session.client_protocol
        )
        if chain is not None:
            session.translator_chain = chain
        return chain

    def _log_disconnect(self, address: str, payload: bytes) -> None:
        """Log the reason from a Disconnect packet."""
        try:
            reader = PacketReader(payload)
            reason = reader.read_varint()
            skip_message = reader.read_bool()
            message = ""
            if not skip_message and reader.has_remaining():
                message = reader.read_string()
            self._logger.warning(
                f"[CB] Disconnect to {address}: reason={reason} "
                f"message={message!r}"
            )
        except Exception:
            self._logger.warning(
                f"[CB] Disconnect to {address}: raw={payload[:64].hex()}"
            )
