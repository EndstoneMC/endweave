"""Protocol factory for v860 (1.21.124) server <- v859 (1.21.120) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v860_to_v859.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)

SERVER_PROTOCOL = 860
CLIENT_PROTOCOL = 859


def create_protocol() -> Protocol:
    """Create a protocol for v860 server <- v859 client translation.

    v859 and v860 share identical packet structures; only the protocol
    version field in login packets differs.
    """
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    protocol.register_serverbound(PacketId.LOGIN, rewrite_login)

    return protocol
