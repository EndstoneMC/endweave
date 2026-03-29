"""Handler for ActorEventPacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    BYTE,
    UVAR_INT64,
    PacketWrapper,
)

_ACTOR_EVENT_KINETIC_DAMAGE_DEALT = 80


def rewrite_actor_event(wrapper: PacketWrapper) -> None:
    """Remap EntityEvent ids for v898 clients.

    Args:
        wrapper: Packet wrapper for EntityEvent.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    event_id = wrapper.read(BYTE)  # Event ID
    remapped = event_id + 1 if event_id >= _ACTOR_EVENT_KINETIC_DAMAGE_DEALT else event_id
    wrapper.write(BYTE, remapped)  # Event ID
