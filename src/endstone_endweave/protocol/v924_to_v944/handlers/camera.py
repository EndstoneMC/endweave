"""Packet handlers for camera-related changes (v924 -> v944).

CameraSplinePacket (338): v944 appends SplineIdentifier + LoadFromJson per spline entry.
CameraInstructionPacket (300): v944 appends SplineIdentifier + LoadFromJson to the spline
sub-message.
"""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    SPLINE_INSTRUCTION_V924,
    SPLINE_INSTRUCTION_V944,
    STRING,
    UVAR_INT,
    VEC2,
    VEC3,
    PacketWrapper,
)

# ---------------------------------------------------------------------------
# CameraSplinePacket (338)
# ---------------------------------------------------------------------------


def rewrite_camera_spline(wrapper: PacketWrapper) -> None:
    """CameraSpline (338): append SplineIdentifier + LoadFromJson per spline, added in v944.

    v944 adds two fields at the end of each CameraSplineDefinition entry:
    - SplineIdentifier (string)
    - LoadFromJson (bool)

    See Also:
        org.cloudburstmc.protocol.bedrock.codec.v944.serializer.CameraSplineSerializer_v944

    Args:
        wrapper: Packet wrapper for CameraSpline.
    """
    spline_count = wrapper.passthrough(UVAR_INT)  # Camera Data Splines
    for _ in range(spline_count):
        wrapper.passthrough(STRING)  # CameraSplineDefinition.name
        wrapper.map(SPLINE_INSTRUCTION_V924, SPLINE_INSTRUCTION_V944)


# ---------------------------------------------------------------------------
# CameraInstructionPacket (300)
# ---------------------------------------------------------------------------


def rewrite_camera_instruction(wrapper: PacketWrapper) -> None:
    """CameraInstruction (300): append SplineIdentifier + LoadFromJson to spline sub-message.

    v944 adds two fields at the end of the optional spline instruction:
    - SplineIdentifier (string)
    - LoadFromJson (bool)

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
        # optional pos (PosOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)  # Pos
        # optional rot (RotOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC2)  # x, y
        # optional facing (FacingOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)  # pos
        # optional view_offset (ViewOffsetOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC2)  # x, y
        # optional entity_offset (EntityOffsetOption)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)  # entity_offset_x/y/z
        # optional default
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(BOOL)
        wrapper.passthrough(BOOL)  # removeIgnoreStartingValuesComponent

    # optional Clear
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)

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
        # optional Target Center Offset
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)
        wrapper.passthrough(INT64_LE)  # Target Actor ID

    # optional RemoveTarget
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)

    # optional FieldOfView (CameraInstruction::FovInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(FLOAT_LE)  # Field of View
        wrapper.passthrough(FLOAT_LE)  # FOV Ease Time
        wrapper.passthrough(BYTE)  # FOV Ease Type
        wrapper.passthrough(BOOL)  # Field of View Clear

    # optional Spline (CameraInstruction::SplineInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.map(SPLINE_INSTRUCTION_V924, SPLINE_INSTRUCTION_V944)

    # optional AttachToEntity (CameraInstruction::AttachToEntityInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(INT64_LE)  # Entity Actor ID

    # optional DetachFromEntity
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)
