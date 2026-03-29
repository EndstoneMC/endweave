"""Handler for GraphicsParameterOverridePacket -- v924 server to v898 client."""

from endstone_endweave.codec import (
    BOOL,
    FLOAT_LE,
    UVAR_INT,
    VEC3,
    PacketWrapper,
)


def rewrite_graphics_parameter_override(wrapper: PacketWrapper) -> None:
    """Strip the v924 float/vector override payloads.

    Args:
        wrapper: Packet wrapper for GraphicsParameterOverride.
    """
    value_count = wrapper.passthrough(UVAR_INT)  # Parameter Keyframe Values
    for _ in range(value_count):
        wrapper.passthrough(FLOAT_LE)  # Float Value
        wrapper.passthrough(VEC3)  # Vec3 Value

    if wrapper.read(BOOL):  # Float Value (strip for v898)
        wrapper.read(FLOAT_LE)

    if wrapper.read(BOOL):  # Vec3 Value (strip for v898)
        wrapper.read(VEC3)
