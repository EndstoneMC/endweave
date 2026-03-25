"""Protocol factory for v859 (1.21.120) server <- v860 (1.21.124) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v859_to_v860.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)

SERVER_PROTOCOL = 859
CLIENT_PROTOCOL = 860


def create_protocol() -> Protocol:
    """Create a protocol for v859 server <- v860 client translation.

    v859 and v860 share identical packet structures; only the protocol
    version field in login packets differs.
    """
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    protocol.register_serverbound(PacketId.LOGIN, rewrite_login)

    return protocol
