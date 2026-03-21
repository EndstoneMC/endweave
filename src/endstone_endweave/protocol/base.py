"""Initial base protocol -- always-on handlers for version detection and disconnect logging."""

from endstone_endweave.codec import BOOL, INT_BE, STRING, UVAR_INT, PacketWrapper
from endstone_endweave.connection import ConnectionState
from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId


def detect_client_protocol(wrapper: PacketWrapper) -> None:
    """Read client protocol from RequestNetworkSettings and store on connection.

    Args:
        wrapper: Packet wrapper for the incoming RequestNetworkSettings packet.
    """
    connection = wrapper.user
    client_proto = wrapper.passthrough(INT_BE)
    connection.client_protocol = client_proto
    connection.state = ConnectionState.LOGIN
    connection.logger.debug(
        f"User connected with protocol: {client_proto} and serverProtocol: {connection.server_protocol}"
    )


def _transition_to_play(wrapper: PacketWrapper) -> None:
    """Transition connection to PLAY state on first StartGame packet.

    Args:
        wrapper: Packet wrapper for the outgoing StartGame packet.
    """
    connection = wrapper.user
    if connection.state != ConnectionState.PLAY:
        connection.state = ConnectionState.PLAY


def log_disconnect(wrapper: PacketWrapper) -> None:
    """Log the reason from a Disconnect packet and mark connection as pending disconnect.

    Args:
        wrapper: Packet wrapper for the outgoing Disconnect packet.
    """
    connection = wrapper.user
    connection.pending_disconnect = True
    try:
        reason = wrapper.passthrough(UVAR_INT)
        skip_message = wrapper.passthrough(BOOL)
        message = ""
        if not skip_message and wrapper.has_remaining:
            message = wrapper.passthrough(STRING)
        connection.logger.info(f"Disconnect {connection.address}: reason={reason} message={message!r}")
    except Exception:
        connection.logger.info(f"Disconnect {connection.address}: could not parse reason")


def create_base_protocol(server_protocol: int) -> Protocol:
    """Create the base protocol that handles version detection and disconnect logging.

    Args:
        server_protocol: The server's protocol version number.

    Returns:
        A Protocol instance with handlers for RequestNetworkSettings
        and Disconnect registered.
    """
    p = Protocol(server_protocol=server_protocol, client_protocol=0, name="base")
    p.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, detect_client_protocol)
    p.register_clientbound(PacketId.START_GAME, _transition_to_play)
    p.register_clientbound(PacketId.DISCONNECT, log_disconnect)
    return p
