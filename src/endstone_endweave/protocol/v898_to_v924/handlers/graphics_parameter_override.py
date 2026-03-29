"""Handler for GraphicsParameterOverridePacket -- v898 server to v924 client."""

from endstone_endweave.codec import (
    BOOL,
    FLOAT_LE,
    UVAR_INT,
    VEC3,
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

    wrapper.write(BOOL, False)  # Float Value (not present)
    wrapper.write(BOOL, False)  # Vec3 Value (not present)
