"""Handler for AnimatePacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    BYTE,
    FLOAT_LE,
    STRING,
    UVAR_INT64,
    OptionalType,
    PacketWrapper,
)


def rewrite_animate_clientbound(wrapper: PacketWrapper) -> None:
    """Append a missing swingSource option for v898 clients.

    Args:
        wrapper: Packet wrapper for Animate.
    """
    wrapper.passthrough_all()
    wrapper.write(OptionalType(STRING), None)  # Swing Source


def rewrite_animate_serverbound(wrapper: PacketWrapper) -> None:
    """Drop the v898 swingSource option for v860 servers.

    Args:
        wrapper: Packet wrapper for Animate.
    """
    wrapper.passthrough(BYTE)  # Action
    wrapper.passthrough(UVAR_INT64)  # RuntimeId
    wrapper.passthrough(FLOAT_LE)  # Data
    wrapper.read(OptionalType(STRING))  # Swing Source
