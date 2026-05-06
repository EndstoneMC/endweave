"""Protocol factory for v860 (1.21.124) server <- v859 (1.21.120) client."""

from endstone_endweave.protocol import Protocol

SERVER_PROTOCOL = 860
CLIENT_PROTOCOL = 859


def create_protocol() -> Protocol:
    """Create a protocol for v860 server <- v859 client translation.

    v859 and v860 share identical packet structures; only the protocol
    version field in login packets differs (handled by base protocol).
    """
    return Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)
