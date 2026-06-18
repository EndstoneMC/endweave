"""Initial base protocol -- always-on handler for version detection and login rewriting.

See Also:
    com.viaversion.viaversion.protocols.base.InitialBaseProtocol
"""

from endstone_endweave.codec import (
    INT_BE,
    STRING,
    VAR_INT,
    PacketViolationSeverity,
    PacketViolationType,
    PacketWrapper,
)

from . import Protocol
from .packet_ids import PacketId


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


def _log_packet_violation(wrapper: PacketWrapper) -> None:
    """Decode PacketViolationWarning and surface it as a warning-level log.

    Forwards the packet unchanged. On parse failure the exception is
    swallowed so the original bytes still reach the client.
    """
    logger = wrapper.user.logger
    try:
        violation_type = wrapper.passthrough(VAR_INT)
        severity = wrapper.passthrough(VAR_INT)
        offending_packet_id = wrapper.passthrough(VAR_INT)
        context = wrapper.passthrough(STRING)
    except Exception as exc:  # noqa: BLE001
        logger.warning(f"Failed to decode PacketViolationWarning: {type(exc).__name__}: {exc}")
        return

    from ..debug import packet_label  # noqa: PLC0415  -- avoid circular import at module load

    try:
        type_name = PacketViolationType(violation_type).name
    except ValueError:
        type_name = f"Type{violation_type}"
    try:
        severity_name = PacketViolationSeverity(severity).name
    except ValueError:
        severity_name = f"Severity{severity}"
    logger.warning(
        f"Server reported packet violation: {type_name}/{severity_name} "
        f"on {packet_label(offending_packet_id)}, context={context!r}"
    )


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
    p.register_clientbound(PacketId.PACKET_VIOLATION_WARNING, _log_packet_violation)
    return p
