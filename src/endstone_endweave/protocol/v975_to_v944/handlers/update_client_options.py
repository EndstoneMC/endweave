"""UpdateClientOptionsPacket (323) -- v944 client to v975 server.

v975 added an optional Filter Profanity Change (bool); append it as absent
so the v975 server gets the field it expects.
"""

from ....codec import BOOL, PacketWrapper


def rewrite_update_client_options(wrapper: PacketWrapper) -> None:
    """Append the missing v975 Filter Profanity Change field.

    Args:
        wrapper: Packet wrapper for UpdateClientOptionsPacket.
    """
    wrapper.passthrough_all()  # Graphics Mode Change (optional)
    wrapper.write(BOOL, False)  # Filter Profanity Change (not present)
