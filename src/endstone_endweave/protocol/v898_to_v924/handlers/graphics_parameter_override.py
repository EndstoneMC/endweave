"""Handler for GraphicsParameterOverridePacket -- v898 server to v924 client."""

from endstone_endweave.codec import (
    FLOAT_LE,
    UVAR_INT,
    VEC3,
    OptionalType,
    PacketWrapper,
)


def rewrite_graphics_parameter_override(wrapper: PacketWrapper) -> None:
    """Insert the v924 Float Value and Vec3 Value optional fields.

    Args:
        wrapper: Packet wrapper for GraphicsParameterOverride.
    """
    value_count = wrapper.passthrough(UVAR_INT)  # Parameter Keyframe Values
    for _ in range(value_count):
        wrapper.passthrough(FLOAT_LE)  # Float Value
        wrapper.passthrough(VEC3)  # Vec3 Value

    wrapper.write(OptionalType(FLOAT_LE), None)  # Float Value (not present)
    wrapper.write(OptionalType(VEC3), None)  # Vec3 Value (not present)
