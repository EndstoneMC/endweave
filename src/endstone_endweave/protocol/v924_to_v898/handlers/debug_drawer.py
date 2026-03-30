"""DebugDrawerPacket handler for v924 to v898.

v924 inserted an optional Attached To Entity ID field inside each
ShapeDataPayload entry, between Dimension ID and Extra Shape Data.
Strip the field so the v898 client sees the format it expects.
"""

from endstone_endweave.codec import (
    BYTE,
    FLOAT_LE,
    INT_LE,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VEC3,
    OptionalType,
    PacketWrapper,
)


def _passthrough_extra_shape_data(wrapper: PacketWrapper) -> None:
    """Passthrough the Extra Shape Data variant.

    Wire format: BYTE type_index + type-specific payload.
    Variant members (from BDS std::variant):
        0 = NullType (no payload)
        1 = ArrowDataPayload
        2 = TextDataPayload
        3 = BoxDataPayload
        4 = LineDataPayload
        5 = SphereDataPayload

    Args:
        wrapper: Packet wrapper positioned at the variant field.
    """
    type_index = wrapper.passthrough(BYTE)  # Extra Shape Data
    if type_index == 1:  # ArrowDataPayload
        wrapper.passthrough(OptionalType(VEC3))  # End Location
        wrapper.passthrough(OptionalType(FLOAT_LE))  # Arrow Head Length
        wrapper.passthrough(OptionalType(FLOAT_LE))  # Arrow Head Radius
        wrapper.passthrough(OptionalType(BYTE))  # Num Segments
    elif type_index == 2:  # TextDataPayload
        wrapper.passthrough(STRING)  # Text
    elif type_index == 3:  # BoxDataPayload
        wrapper.passthrough(VEC3)  # Box Bound
    elif type_index == 4:  # LineDataPayload
        wrapper.passthrough(VEC3)  # End Location
    elif type_index == 5:  # SphereDataPayload
        wrapper.passthrough(BYTE)  # Num Segments


def _passthrough_shape_common(wrapper: PacketWrapper) -> None:
    """Passthrough ShapeDataPayload fields shared between v898 and v924.

    Handles fields 1-8 (NetworkId through Dimension ID) and field 10
    (Extra Shape Data variant). The caller handles field 9 (Attached To
    Entity ID) between Dimension ID and Extra Shape Data.

    Args:
        wrapper: Packet wrapper positioned at the start of a shape entry.
    """
    wrapper.passthrough(UVAR_INT64)  # NetworkId
    wrapper.passthrough(OptionalType(BYTE))  # Shape Type
    wrapper.passthrough(OptionalType(VEC3))  # Location
    wrapper.passthrough(OptionalType(FLOAT_LE))  # Scale
    wrapper.passthrough(OptionalType(VEC3))  # Rotation
    wrapper.passthrough(OptionalType(FLOAT_LE))  # Total Time Left
    wrapper.passthrough(OptionalType(INT_LE))  # Color
    wrapper.passthrough(VAR_INT)  # Dimension ID


def rewrite_debug_drawer(wrapper: PacketWrapper) -> None:
    """Strip Attached To Entity ID from each ShapeDataPayload.

    Args:
        wrapper: Packet wrapper for DebugDrawerPacket.
    """
    shape_count = wrapper.passthrough(UVAR_INT)  # Array of debug shapes
    for _ in range(shape_count):
        _passthrough_shape_common(wrapper)
        wrapper.read(OptionalType(UVAR_INT64))  # Attached To Entity ID (strip)
        _passthrough_extra_shape_data(wrapper)
