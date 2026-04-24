"""ActorEventPacket (27) -- v975 server to v944 client.

v975 appended an optional Fire At Position (Vec3) field at the end;
strip it so the v944 client sees its expected layout.
"""

from ....codec import BYTE, OPTIONAL_VEC3, UVAR_INT64, VAR_INT, PacketWrapper


def rewrite_actor_event(wrapper: PacketWrapper) -> None:
    """Strip the v975-only Fire At Position field.

    Args:
        wrapper: Packet wrapper for ActorEventPacket.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(BYTE)  # Event ID
    wrapper.passthrough(VAR_INT)  # Data
    wrapper.read(OPTIONAL_VEC3)  # Fire At Position (strip)
