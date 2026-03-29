"""ActorEvent packet handler for v898 server to v860 client translation."""

from endstone_endweave.codec import (
    BYTE,
    UVAR_INT64,
    PacketWrapper,
)

_ACTOR_EVENT_KINETIC_DAMAGE_DEALT = 80


def rewrite_actor_event(wrapper: PacketWrapper) -> None:
    """Remap EntityEvent ids for v860 clients.

    Args:
        wrapper: Packet wrapper for EntityEvent.
    """
    wrapper.passthrough(UVAR_INT64)
    event_id = wrapper.read(BYTE)
    if event_id == _ACTOR_EVENT_KINETIC_DAMAGE_DEALT:
        wrapper.cancel()
        return
    remapped = event_id - 1 if event_id > _ACTOR_EVENT_KINETIC_DAMAGE_DEALT else event_id
    wrapper.write(BYTE, remapped)
