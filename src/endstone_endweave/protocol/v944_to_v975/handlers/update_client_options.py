"""UpdateClientOptionsPacket (323) -- v975 client to v944 server.

v975 appended an optional Filter Profanity Change (bool) at the end.
Strip it so the v944 server only sees the fields it expects.
"""

from endstone_endweave.codec import BOOL, BYTE, PacketWrapper


def rewrite_update_client_options(wrapper: PacketWrapper) -> None:
    """Strip the v975-only Filter Profanity Change field.

    Args:
        wrapper: Packet wrapper for UpdateClientOptionsPacket.
    """
    # Graphics Mode Change (optional uint8)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BYTE)  # Graphics Mode Change value
    # Filter Profanity Change (optional bool) -- strip
    if wrapper.read(BOOL):
        wrapper.read(BOOL)  # Filter Profanity Change value
