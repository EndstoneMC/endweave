"""ResourcePackStack packet handler for v898 server to v860 client translation."""

from ....codec import (
    BOOL,
    STRING,
    UVAR_INT,
    PacketWrapper,
)


def rewrite_resource_pack_stack(wrapper: PacketWrapper) -> None:
    """Insert the removed behavior pack array for v860 clients.

    Args:
        wrapper: Packet wrapper for ResourcePackStack.
    """
    wrapper.passthrough(BOOL)  # Texture Pack Required
    wrapper.write(UVAR_INT, 0)  # Add-On List

    resource_pack_count = wrapper.passthrough(UVAR_INT)  # Texture Pack List
    for _ in range(resource_pack_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
        wrapper.passthrough(STRING)
