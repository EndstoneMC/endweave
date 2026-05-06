"""Handler for AnimatePacket -- v860 server to v898 client.

v860 uses varint for Action and has a conditional Rowing Time float
when Action & 0x80. v898 uses uint8 for Action and optional SwingSource.
"""

from endstone_endweave.codec import (
    BYTE,
    FLOAT_LE,
    STRING,
    UVAR_INT64,
    VAR_INT,
    OptionalType,
    PacketWrapper,
)

_ROWING_FLAG = 0x80


def rewrite_animate_clientbound(wrapper: PacketWrapper) -> None:
    """Convert v860 AnimatePacket to v898 format (clientbound).

    Args:
        wrapper: Packet wrapper for Animate.
    """
    action = wrapper.read(VAR_INT)  # Action (varint in v860)
    wrapper.write(BYTE, action)  # Action (uint8 in v898)
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(FLOAT_LE)  # Data
    if action & _ROWING_FLAG:
        wrapper.read(FLOAT_LE)  # Rowing Time (strip for v898)
    wrapper.write(OptionalType(STRING), None)  # Swing Source


def rewrite_animate_serverbound(wrapper: PacketWrapper) -> None:
    """Convert v898 AnimatePacket to v860 format (serverbound).

    Args:
        wrapper: Packet wrapper for Animate.
    """
    action = wrapper.read(BYTE)  # Action (uint8 in v898)
    wrapper.write(VAR_INT, action)  # Action (varint in v860)
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(FLOAT_LE)  # Data
    wrapper.read(OptionalType(STRING))  # Swing Source (strip for v860)
