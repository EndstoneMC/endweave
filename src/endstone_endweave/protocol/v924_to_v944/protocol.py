"""Protocol factory for v924 (r26_u0) server <- v944 (r26_u1) client."""

from __future__ import annotations

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v924_to_v944.handlers.login import rewrite_login, rewrite_request_network_settings

SERVER_PROTOCOL = 924
CLIENT_PROTOCOL = 944


def create_protocol() -> Protocol:
    """Create a protocol for v924 server <- v944 client."""
    p = Protocol(server_protocol=924, client_protocol=944)

    # Login
    p.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    p.register_serverbound(PacketId.LOGIN, rewrite_login)

    # Cancel new v944 serverbound packets unknown to v924 (v924 EndId = 340)
    p.cancel_serverbound(
        *range(PacketId.RESOURCE_PACKS_READY_FOR_VALIDATION, PacketId.CLIENTBOUND_ATTRIBUTE_LAYER_SYNC + 1)
    )
    return p
