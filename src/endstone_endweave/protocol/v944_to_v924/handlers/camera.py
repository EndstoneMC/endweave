"""Packet handlers for camera-related changes (v944 -> v924)."""

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

_CAMERA_EASES = {
    "linear": 0,
    "spring": 1,
    "in_sine": 2,
    "out_sine": 3,
    "in_out_sine": 4,
    "in_quad": 5,
    "out_quad": 6,
    "in_out_quad": 7,
    "in_cubic": 8,
    "out_cubic": 9,
    "in_out_cubic": 10,
    "in_quart": 11,
    "out_quart": 12,
    "in_out_quart": 13,
    "in_quint": 14,
    "out_quint": 15,
    "in_out_quint": 16,
    "in_expo": 17,
    "out_expo": 18,
    "in_out_expo": 19,
    "in_circ": 20,
    "out_circ": 21,
    "in_out_circ": 22,
    "in_back": 23,
    "out_back": 24,
    "in_out_back": 25,
    "in_elastic": 26,
    "out_elastic": 27,
    "in_out_elastic": 28,
    "in_bounce": 29,
    "out_bounce": 30,
    "in_out_bounce": 31,
}


def _write_camera_ease(wrapper: PacketWrapper, value: str) -> None:
    wrapper.write(BYTE, _CAMERA_EASES[value])


def _passthrough_spline_instruction(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(BYTE)
    curve_count = wrapper.passthrough(UVAR_INT)
    for _ in range(curve_count):
        wrapper.passthrough(VEC3)
    progress_count = wrapper.passthrough(UVAR_INT)
    for _ in range(progress_count):
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
        _write_camera_ease(wrapper, wrapper.read(STRING))
    rotation_count = wrapper.passthrough(UVAR_INT)
    for _ in range(rotation_count):
        wrapper.passthrough(VEC3)
        wrapper.passthrough(FLOAT_LE)
        _write_camera_ease(wrapper, wrapper.read(STRING))
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)


def rewrite_camera_spline(wrapper: PacketWrapper) -> None:
    """CameraSpline (338): strip SplineIdentifier + LoadFromJson.

    Args:
        wrapper: Packet wrapper for CameraSpline.
    """
    spline_count = wrapper.passthrough(UVAR_INT)
    for _ in range(spline_count):
        wrapper.passthrough(STRING)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(STRING)
        curve_count = wrapper.passthrough(UVAR_INT)
        for _ in range(curve_count):
            wrapper.passthrough(VEC3)
        progress_count = wrapper.passthrough(UVAR_INT)
        for _ in range(progress_count):
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(STRING)
        rotation_count = wrapper.passthrough(UVAR_INT)
        for _ in range(rotation_count):
            wrapper.passthrough(VEC3)
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(STRING)
        wrapper.read(STRING)
        wrapper.read(BOOL)


def rewrite_camera_instruction(wrapper: PacketWrapper) -> None:
    """CameraInstruction (300): rewrite eased fields into the v924 form.

    Args:
        wrapper: Packet wrapper for CameraInstruction.
    """
    if wrapper.passthrough(BOOL):
        wrapper.passthrough(INT_LE)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(BYTE)
            wrapper.passthrough(FLOAT_LE)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC2)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC2)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(BOOL)
        wrapper.passthrough(BOOL)

    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)

    if wrapper.passthrough(BOOL):
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(FLOAT_LE)
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(FLOAT_LE)
            wrapper.passthrough(FLOAT_LE)

    if wrapper.passthrough(BOOL):
        if wrapper.passthrough(BOOL):
            wrapper.passthrough(VEC3)
        wrapper.passthrough(INT64_LE)

    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)

    if wrapper.passthrough(BOOL):
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
        _write_camera_ease(wrapper, wrapper.read(STRING))
        wrapper.passthrough(BOOL)

    if wrapper.passthrough(BOOL):
        _passthrough_spline_instruction(wrapper)

    if wrapper.passthrough(BOOL):
        wrapper.passthrough(INT64_LE)

    if wrapper.passthrough(BOOL):
        wrapper.passthrough(BOOL)
