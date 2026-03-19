"""Base protocol -- always-on handlers for version detection and disconnect logging."""

from __future__ import annotations

from endstone_endweave.codec import UVAR_INT, BOOL, STRING, INT_BE, PacketWrapper
from endstone_endweave.protocol.base import Protocol
from endstone_endweave.protocol.packet_ids import PacketId


def detect_client_protocol(wrapper: PacketWrapper) -> None:
    """Read client protocol from RequestNetworkSettings and store on connection."""
    connection = wrapper.user()
    client_proto = wrapper.passthrough(INT_BE)
    connection.client_protocol = client_proto
    connection.logger.debug(
        f"RequestNetworkSettings: detected client protocol {client_proto}"
    )


def log_disconnect(wrapper: PacketWrapper) -> None:
    """Log the reason from a Disconnect packet (does not modify payload)."""
    connection = wrapper.user()
    try:
        reason = wrapper.passthrough(UVAR_INT)
        skip_message = wrapper.passthrough(BOOL)
        message = ""
        if not skip_message and wrapper.has_remaining():
            message = wrapper.passthrough(STRING)
        connection.logger.warning(
            f"Disconnect to {connection.address}: reason={reason} message={message!r}"
        )
    except Exception:
        connection.logger.warning(
            f"Disconnect to {connection.address}: could not parse"
        )


def create_base_protocol(server_protocol: int) -> Protocol:
    """Create the base protocol that handles version detection and disconnect logging."""
    p = Protocol(server_protocol=server_protocol, client_protocol=0)
    p.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, detect_client_protocol)
    p.register_clientbound(PacketId.DISCONNECT, log_disconnect)
    return p
