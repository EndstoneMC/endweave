"""Handler for GraphicsParameterOverridePacket -- v924 server to v898 client."""

from endstone_endweave.codec import (
    FLOAT_LE,
    UVAR_INT,
    VEC3,
    OptionalType,
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

    wrapper.read(OptionalType(FLOAT_LE))  # Float Value (strip for v898)
    wrapper.read(OptionalType(VEC3))  # Vec3 Value (strip for v898)
