"""Handler for ResourcePackStackPacket -- v860 server to v898 client."""

from endstone_endweave.codec import (
    BOOL,
    STRING,
    UVAR_INT,
    PacketWrapper,
)


def rewrite_resource_pack_stack(wrapper: PacketWrapper) -> None:
    """Drop the removed behavior pack array from ResourcePackStack.

    Args:
        wrapper: Packet wrapper for ResourcePackStack.
    """
    wrapper.passthrough(BOOL)  # Texture Pack Required

    behavior_pack_count = wrapper.read(UVAR_INT)  # Add-On List
    for _ in range(behavior_pack_count):
        wrapper.read(STRING)
        wrapper.read(STRING)
        wrapper.read(STRING)

    resource_pack_count = wrapper.passthrough(UVAR_INT)  # Texture Pack List
    for _ in range(resource_pack_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
