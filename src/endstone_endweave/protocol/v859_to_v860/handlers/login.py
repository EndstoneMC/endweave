"""Handlers for login-phase packets (RequestNetworkSettings, Login)."""

from endstone_endweave.codec import INT_BE, PacketWrapper


def rewrite_request_network_settings(wrapper: PacketWrapper) -> None:
    """Rewrite RequestNetworkSettings to the server protocol.

    Args:
        wrapper: Packet wrapper for RequestNetworkSettings.
    """
    connection = wrapper.user
    client_protocol = wrapper.read(INT_BE)
    target = connection.server_protocol if client_protocol != connection.server_protocol else client_protocol
    wrapper.write(INT_BE, target)


def rewrite_login(wrapper: PacketWrapper) -> None:
    """Rewrite the Login packet's protocol version.

    Args:
        wrapper: Packet wrapper for Login.
    """
    connection = wrapper.user
    client_protocol = wrapper.read(INT_BE)
    target = connection.server_protocol if client_protocol != connection.server_protocol else client_protocol
    wrapper.write(INT_BE, target)
