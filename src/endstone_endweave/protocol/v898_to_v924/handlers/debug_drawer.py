"""DebugDrawerPacket handler for v898 to v924.

v924 inserted an optional Attached To Entity ID field inside each
ShapeDataPayload entry, between Dimension ID and Extra Shape Data.
Insert the field (as absent) so the v924 client sees the format it expects.
"""

from ....codec import (
    BOOL,
    UVAR_INT,
    PacketWrapper,
)
from ...v924_to_v898.handlers.debug_drawer import (
    _passthrough_extra_shape_data,
    _passthrough_shape_common,
)


def rewrite_debug_drawer(wrapper: PacketWrapper) -> None:
    """Insert absent Attached To Entity ID into each ShapeDataPayload.

    Args:
        wrapper: Packet wrapper for DebugDrawerPacket.
    """
    shape_count = wrapper.passthrough(UVAR_INT)  # Array of debug shapes
    for _ in range(shape_count):
        _passthrough_shape_common(wrapper)
        wrapper.write(BOOL, False)  # Attached To Entity ID (absent)
        _passthrough_extra_shape_data(wrapper)
