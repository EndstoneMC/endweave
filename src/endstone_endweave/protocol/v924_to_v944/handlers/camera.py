"""Packet handlers for camera-related changes (v924 -> v944)."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT64_LE,
    INT_LE,
    STRING,
    UVAR_INT,
    VEC2,
    VEC3,
    PacketWrapper,
)

_CAMERA_EASES = (
    "linear",
    "spring",
    "in_sine",
    "out_sine",
    "in_out_sine",
    "in_quad",
    "out_quad",
    "in_out_quad",
    "in_cubic",
    "out_cubic",
    "in_out_cubic",
    "in_quart",
    "out_quart",
    "in_out_quart",
    "in_quint",
    "out_quint",
    "in_out_quint",
    "in_expo",
    "out_expo",
    "in_out_expo",
    "in_circ",
    "out_circ",
    "in_out_circ",
    "in_back",
    "out_back",
    "in_out_back",
    "in_elastic",
    "out_elastic",
    "in_out_elastic",
    "in_bounce",
    "out_bounce",
    "in_out_bounce",
)


def _write_camera_ease(wrapper: PacketWrapper, value: int) -> None:
    wrapper.write(STRING, _CAMERA_EASES[value])


def _passthrough_spline_instruction(wrapper: PacketWrapper) -> None:
    """Passthrough a CameraInstruction::SplineInstruction."""
    wrapper.passthrough(FLOAT_LE)  # totalTime
    wrapper.passthrough(BYTE)  # type
    # curve
    curve_count = wrapper.passthrough(UVAR_INT)
    for _ in range(curve_count):
        wrapper.passthrough(VEC3)
    # progressKeyFrames
    pk_count = wrapper.passthrough(UVAR_INT)
    for _ in range(pk_count):
        wrapper.passthrough(FLOAT_LE)  # Key frame value
        wrapper.passthrough(FLOAT_LE)  # Key frame time
        _write_camera_ease(wrapper, wrapper.read(BYTE))  # Key frame easing func
    # rotationOption
    rk_count = wrapper.passthrough(UVAR_INT)
    for _ in range(rk_count):
        wrapper.passthrough(VEC3)  # Key frame value
        wrapper.passthrough(FLOAT_LE)  # Key frame time
        _write_camera_ease(wrapper, wrapper.read(BYTE))  # Key frame easing func
    wrapper.passthrough(STRING)  # splineIdentifier
    wrapper.passthrough(BOOL)  # loadFromJson


# ---------------------------------------------------------------------------
# CameraSplinePacket (338)
# ---------------------------------------------------------------------------


def rewrite_camera_spline(wrapper: PacketWrapper) -> None:
    """CameraSpline (338): append SplineIdentifier + LoadFromJson per spline."""
    spline_count = wrapper.passthrough(UVAR_INT)  # Camera Data Splines
    for _ in range(spline_count):
        wrapper.passthrough(STRING)  # CameraSplineDefinition.name
        wrapper.passthrough(FLOAT_LE)  # totalTime
        wrapper.passthrough(STRING)  # type
        curve_count = wrapper.passthrough(UVAR_INT)
        for _ in range(curve_count):
            wrapper.passthrough(VEC3)
        pk_count = wrapper.passthrough(UVAR_INT)
        for _ in range(pk_count):
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(STRING)
        rk_count = wrapper.passthrough(UVAR_INT)
        for _ in range(rk_count):
            wrapper.passthrough(VEC3)
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(STRING)
        wrapper.write(STRING, "")  # splineIdentifier
        wrapper.write(BOOL, False)  # loadFromJson


# ---------------------------------------------------------------------------
# CameraInstructionPacket (300)
# ---------------------------------------------------------------------------


def rewrite_camera_instruction(wrapper: PacketWrapper) -> None:
    """CameraInstruction (300): rewrite eased fields into the v944 form."""
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
        _write_camera_ease(wrapper, wrapper.read(BYTE))  # FOV Ease Type
        wrapper.passthrough(BOOL)  # Field of View Clear

    # optional Spline (CameraInstruction::SplineInstruction)
    if wrapper.passthrough(BOOL):
        _passthrough_spline_instruction(wrapper)

    # optional AttachToEntity (CameraInstruction::AttachToEntityInstruction)
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(INT64_LE)  # Entity Actor ID

    # optional DetachFromEntity
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)
