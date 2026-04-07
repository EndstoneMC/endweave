"""Handler for InteractPacket -- v860 server to v898 client."""

from ....codec import (
    BOOL,
    BYTE,
    UVAR_INT64,
    VEC3,
    InteractAction,
    PacketWrapper,
)

_HAS_POSITION = (InteractAction.INTERACT_UPDATE, InteractAction.STOP_RIDING)


def rewrite_interact(wrapper: PacketWrapper) -> None:
    """Convert v898 optional mousePosition back to the v860 form.

    Args:
        wrapper: Packet wrapper for Interact.
    """
    action = wrapper.passthrough(BYTE)  # Action
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID

    has_mouse_position = wrapper.read(BOOL)
    if action in _HAS_POSITION:
        if has_mouse_position:
            wrapper.passthrough(VEC3)  # Position
        else:
            wrapper.write(VEC3, (0.0, 0.0, 0.0))  # Position
    elif has_mouse_position:
        wrapper.read(VEC3)  # Position
