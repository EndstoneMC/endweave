"""Protocol factory for v859 (1.21.120) server <- v860 (1.21.124) client."""

from .. import Protocol

SERVER_PROTOCOL = 859
CLIENT_PROTOCOL = 860


def create_protocol() -> Protocol:
    """Create a protocol for v859 server <- v860 client translation.

    v859 and v860 share identical packet structures; only the protocol
    version field in login packets differs (handled by base protocol).
    """
    return Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)
