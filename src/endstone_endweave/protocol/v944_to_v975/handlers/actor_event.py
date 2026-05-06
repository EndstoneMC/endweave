"""ActorEventPacket (27) -- v944 server to v975 client.

v975 appended an optional Fire At Position (Vec3) field at the end.
"""

from endstone_endweave.codec import BOOL, PacketWrapper


def rewrite_actor_event(wrapper: PacketWrapper) -> None:
    """Append the missing v975 Fire At Position field.

    Args:
        wrapper: Packet wrapper for ActorEventPacket.
    """
    wrapper.passthrough_all()  # Target Runtime ID, Event ID, Data
    wrapper.write(BOOL, False)  # Fire At Position (not present)
