"""Initial base protocol -- always-on handler for version detection and login rewriting.

See Also:
    com.viaversion.viaversion.protocols.base.InitialBaseProtocol
"""

from endstone_endweave.codec import INT_BE, PacketWrapper
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId


def detect_client_protocol(wrapper: PacketWrapper) -> None:
    """Read client protocol from RequestNetworkSettings, store on connection, and rewrite to server protocol.

    Args:
        wrapper: Packet wrapper for the incoming RequestNetworkSettings packet.
    """
    connection = wrapper.user
    client_proto = wrapper.read(INT_BE)  # ClientNetworkVersion
    connection.client_protocol = client_proto
    wrapper.write(INT_BE, connection.server_protocol)  # ClientNetworkVersion
    connection.logger.debug(
        f"User connected with protocol: {client_proto} and serverProtocol: {connection.server_protocol}"
    )


def _rewrite_login(wrapper: PacketWrapper) -> None:
    """Rewrite the Login packet's protocol version to the server protocol.

    Args:
        wrapper: Packet wrapper for the incoming Login packet.
    """
    connection = wrapper.user
    wrapper.read(INT_BE)  # Client Network Version
    wrapper.write(INT_BE, connection.server_protocol)  # Client Network Version


def create_base_protocol(server_protocol: int) -> Protocol:
    """Create the base protocol that handles version detection and login rewriting.

    Args:
        server_protocol: The server's protocol version number.

    Returns:
        A Protocol instance with handlers for RequestNetworkSettings and Login.
    """
    p = Protocol(server_protocol=server_protocol, client_protocol=0, name="base", is_base=True)
    p.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, detect_client_protocol)
    p.register_serverbound(PacketId.LOGIN, _rewrite_login)
    return p
