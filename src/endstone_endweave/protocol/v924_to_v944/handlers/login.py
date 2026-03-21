"""Handlers for login-phase packets (RequestNetworkSettings, Login)."""

from endstone_endweave.codec import INT_BE, PacketWrapper


def rewrite_request_network_settings(wrapper: PacketWrapper) -> None:
    """Rewrite the client's protocol version to match the server's.

    RequestNetworkSettings payload:
    - int32 BE: ClientNetworkVersion (protocol number)

    Args:
        wrapper: Packet wrapper for RequestNetworkSettings.
    """
    connection = wrapper.user
    client_protocol = wrapper.read(INT_BE)  # ClientNetworkVersion

    if client_protocol == connection.server_protocol:
        connection.logger.debug(f"RequestNetworkSettings: protocol {client_protocol} matches server, no rewrite")
        wrapper.write(INT_BE, client_protocol)  # ClientNetworkVersion
        return

    connection.logger.debug(f"RequestNetworkSettings: protocol {client_protocol} -> {connection.server_protocol}")
    wrapper.write(INT_BE, connection.server_protocol)  # ClientNetworkVersion


def rewrite_login(wrapper: PacketWrapper) -> None:
    """Rewrite the Login packet's protocol version.

    Login payload:
    - int32 BE: Client Network Version (protocol number)
    - bytes: Connection Request (JWT chain data)

    Args:
        wrapper: Packet wrapper for Login.
    """
    connection = wrapper.user
    protocol_in_packet = wrapper.read(INT_BE)  # Client Network Version

    if protocol_in_packet == connection.server_protocol:
        connection.logger.debug(f"Login: protocol {protocol_in_packet} matches server, no rewrite")
        wrapper.write(INT_BE, protocol_in_packet)  # Client Network Version
        return

    connection.logger.debug(f"Login: protocol {protocol_in_packet} -> {connection.server_protocol}")
    wrapper.write(INT_BE, connection.server_protocol)  # Client Network Version
