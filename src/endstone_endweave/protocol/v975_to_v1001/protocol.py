"""Protocol factory for v975 (r26_u2) server <- v1001 (r26_u3) client.

Currently no packet-level transforms are required for v1001 vs v975; register a
pass-through Protocol so the pipeline can route newer clients to older servers.
"""

from endstone_endweave.protocol import Protocol

SERVER_PROTOCOL = 975
CLIENT_PROTOCOL = 1001


def create_protocol() -> Protocol:
    """Create a protocol for v975 server <- v1001 client.

    Returns:
        A Protocol instance. No handlers are registered by default.
    """
    return Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL, name="v975_to_v1001")
