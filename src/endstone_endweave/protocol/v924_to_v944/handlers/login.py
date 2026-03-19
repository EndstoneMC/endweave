"""Handlers for login-phase packets (RequestNetworkSettings, Login)."""

from __future__ import annotations

from endstone_endweave.codec import INT_BE, PacketWrapper


def rewrite_request_network_settings(wrapper: PacketWrapper) -> None:
    """Rewrite the client's protocol version to match the server's.

    RequestNetworkSettings payload:
    - int32 BE: client_network_version (protocol number)
    """
    connection = wrapper.user()
    client_protocol = wrapper.read(INT_BE)

    if client_protocol == connection.server_protocol:
        connection.logger.debug(
            f"RequestNetworkSettings: protocol {client_protocol} matches server, no rewrite"
        )
        wrapper.write(INT_BE, client_protocol)
        return

    connection.logger.debug(
        f"RequestNetworkSettings: protocol {client_protocol} -> {connection.server_protocol}"
    )
    wrapper.write(INT_BE, connection.server_protocol)


def rewrite_login(wrapper: PacketWrapper) -> None:
    """Rewrite the Login packet's protocol version.

    Login payload:
    - int32 BE: protocol_version
    - bytes: JWT chain data
    """
    connection = wrapper.user()
    protocol_in_packet = wrapper.read(INT_BE)

    if protocol_in_packet == connection.server_protocol:
        connection.logger.debug(
            f"Login: protocol {protocol_in_packet} matches server, no rewrite"
        )
        wrapper.write(INT_BE, protocol_in_packet)
        return

    connection.logger.debug(
        f"Login: protocol {protocol_in_packet} -> {connection.server_protocol}"
    )
    wrapper.write(INT_BE, connection.server_protocol)
