"""Interact packet handler for v898 server to v860 client translation."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    UVAR_INT64,
    InteractAction,
    PacketWrapper,
)

_HAS_POSITION = (InteractAction.INTERACT_UPDATE, InteractAction.STOP_RIDING)


def rewrite_interact(wrapper: PacketWrapper) -> None:
    """Convert the v860 mousePosition into the v898 optional form.

    Args:
        wrapper: Packet wrapper for Interact.
    """
    action = wrapper.passthrough(BYTE)  # Action
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID

    if action in _HAS_POSITION:
        wrapper.write(BOOL, True)
    else:
        wrapper.write(BOOL, False)
