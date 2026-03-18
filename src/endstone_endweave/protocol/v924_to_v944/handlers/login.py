"""Handlers for login-phase packets (RequestNetworkSettings, Login)."""

from __future__ import annotations

import struct

from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation


def rewrite_request_network_settings(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Rewrite the client's protocol version to match the server's.

    RequestNetworkSettings payload:
    - int32 BE: client_network_version (protocol number)
    """
    if len(payload) < 4:
        return PacketTransformation()

    client_protocol = struct.unpack(">i", payload[:4])[0]
    session.client_protocol = client_protocol

    if client_protocol == session.server_protocol:
        return PacketTransformation()

    new_payload = struct.pack(">i", session.server_protocol) + payload[4:]
    return PacketTransformation(new_payload=new_payload)


def rewrite_login(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Rewrite the Login packet's protocol version.

    Login payload:
    - int32 BE: protocol_version
    - bytes: JWT chain data
    """
    if len(payload) < 4:
        return PacketTransformation()

    protocol_in_packet = struct.unpack(">i", payload[:4])[0]

    if protocol_in_packet == session.server_protocol:
        return PacketTransformation()

    new_payload = struct.pack(">i", session.server_protocol) + payload[4:]
    return PacketTransformation(new_payload=new_payload)
