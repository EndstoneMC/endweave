"""Packet handler for CameraInstruction (300) -- v898 server to v924 client.

v924 added easing functions to spline keyframes. Default easing (0 = linear)
is injected for v898 splines.
"""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    OPTIONAL_BOOL,
    OPTIONAL_VEC2,
    OPTIONAL_VEC3,
    SPLINE_INSTRUCTION_V898,
    SPLINE_INSTRUCTION_V924,
    OptionalType,
    PacketWrapper,
)


def rewrite_camera_instruction(wrapper: PacketWrapper) -> None:
    """CameraInstruction (300): inject default easing into spline keyframes for v924 clients.

    Args:
        wrapper: Packet wrapper for CameraInstruction.
    """
    # optional Set (CameraInstruction::SetInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(INT_LE)  # preset
        # optional ease (EaseOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(BYTE)  # type
            wrapper.passthrough(FLOAT_LE)  # time
        wrapper.passthrough(OPTIONAL_VEC3)  # pos (PosOption)
        wrapper.passthrough(OPTIONAL_VEC2)  # rot (RotOption)
        wrapper.passthrough(OPTIONAL_VEC3)  # facing (FacingOption)
        wrapper.passthrough(OPTIONAL_VEC2)  # view_offset (ViewOffsetOption)
        wrapper.passthrough(OPTIONAL_VEC3)  # entity_offset (EntityOffsetOption)
        wrapper.passthrough(OPTIONAL_BOOL)  # default
        wrapper.passthrough(BOOL)  # removeIgnoreStartingValuesComponent

    wrapper.passthrough(OPTIONAL_BOOL)  # Clear

    # optional Fade (CameraInstruction::FadeInstruction)
    if wrapper.passthrough(BOOL):
        # optional Time (TimeOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(FLOAT_LE)  # Fade In Time
            wrapper.passthrough(FLOAT_LE)  # Hold Time
            wrapper.passthrough(FLOAT_LE)  # Fade Out Time
        # optional Color (ColorOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(FLOAT_LE)  # Red
            wrapper.passthrough(FLOAT_LE)  # Green
            wrapper.passthrough(FLOAT_LE)  # Blue

    # optional Target (CameraInstruction::TargetInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(OPTIONAL_VEC3)  # Target Center Offset
        wrapper.passthrough(INT64_LE)  # Target Actor ID

    wrapper.passthrough(OPTIONAL_BOOL)  # RemoveTarget

    # optional FieldOfView (CameraInstruction::FovInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(FLOAT_LE)  # Field of View
        wrapper.passthrough(FLOAT_LE)  # FOV Ease Time
        wrapper.passthrough(BYTE)  # FOV Ease Type
        wrapper.passthrough(BOOL)  # Field of View Clear

    # optional Spline (CameraInstruction::SplineInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.map(SPLINE_INSTRUCTION_V898, SPLINE_INSTRUCTION_V924)

    wrapper.passthrough(OptionalType(INT64_LE))  # AttachToEntity
    wrapper.passthrough(OPTIONAL_BOOL)  # DetachFromEntity
