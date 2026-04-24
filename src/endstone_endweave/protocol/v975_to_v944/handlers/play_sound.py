"""PlaySoundPacket (86) -- v975 server to v944 client.

v975 appended an optional Server Sound Handle (uint64) at the end;
strip it so the v944 client sees its expected layout.
"""

from ....codec import BLOCK_POS, BOOL, FLOAT_LE, INT64_LE, STRING, PacketWrapper


def rewrite_play_sound(wrapper: PacketWrapper) -> None:
    """Strip the v975-only Server Sound Handle field.

    Args:
        wrapper: Packet wrapper for PlaySoundPacket.
    """
    wrapper.passthrough(STRING)  # Name
    wrapper.passthrough(BLOCK_POS)  # Position
    wrapper.passthrough(FLOAT_LE)  # Volume
    wrapper.passthrough(FLOAT_LE)  # Pitch
    if wrapper.read(BOOL):  # Server Sound Handle (optional)
        wrapper.read(INT64_LE)  # Server Sound Handle value
