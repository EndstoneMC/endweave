"""PlaySoundPacket (86) -- v944 server to v975 client.

v975 appended an optional Server Sound Handle (uint64) at the end.
"""

from endstone_endweave.codec import BOOL, PacketWrapper


def rewrite_play_sound(wrapper: PacketWrapper) -> None:
    """Append the missing v975 Server Sound Handle field.

    Args:
        wrapper: Packet wrapper for PlaySoundPacket.
    """
    wrapper.passthrough_all()  # Name, Position, Volume, Pitch
    wrapper.write(BOOL, False)  # Server Sound Handle (not present)
