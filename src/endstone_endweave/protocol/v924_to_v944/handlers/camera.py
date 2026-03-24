"""Packet handlers for camera-related changes (v924 -> v944).

CameraSplinePacket (338): v944 appends SplineIdentifier + LoadFromJson per spline entry.
"""

from endstone_endweave.codec import (
    BOOL,
    FLOAT_LE,
    STRING,
    UVAR_INT,
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
        wrapper.passthrough(FLOAT_LE)  # CameraSplineDefinition.total_time
        wrapper.passthrough(STRING)  # CameraSplineDefinition.spline_type
        # CameraSplineDefinition.control_points
        cp_count = wrapper.passthrough(UVAR_INT)
        for _ in range(cp_count):
            wrapper.passthrough(FLOAT_LE)  # position.X
            wrapper.passthrough(FLOAT_LE)  # position.Y
            wrapper.passthrough(FLOAT_LE)  # position.Z
        # CameraSplineDefinition.progress_key_frames
        pk_count = wrapper.passthrough(UVAR_INT)
        for _ in range(pk_count):
            wrapper.passthrough(FLOAT_LE)  # progress
            wrapper.passthrough(FLOAT_LE)  # time
            wrapper.passthrough(STRING)  # easing
        # CameraSplineDefinition.rotation_key_frames
        rk_count = wrapper.passthrough(UVAR_INT)
        for _ in range(rk_count):
            wrapper.passthrough(FLOAT_LE)  # rotation.X
            wrapper.passthrough(FLOAT_LE)  # rotation.Y
            wrapper.passthrough(FLOAT_LE)  # rotation.Z
            wrapper.passthrough(FLOAT_LE)  # time
            wrapper.passthrough(STRING)  # easing
        # v944 additions per CameraSplineDefinition
        wrapper.write(STRING, "")  # splineIdentifier
        wrapper.write(BOOL, False)  # loadFromJson
