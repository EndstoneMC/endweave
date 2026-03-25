"""Camera-related compound types (SplineInstruction)."""

from dataclasses import dataclass, field

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.types.primitives import (
    BOOL,
    BYTE,
    FLOAT_LE,
    INT_LE,
    STRING,
    UVAR_INT,
    VEC3,
    Type,
)
from endstone_endweave.codec.writer import PacketWriter


@dataclass
class KeyFrame:
    """A progress key frame in a camera spline.

    Attributes:
        value: Key frame value.
        time: Key frame time.
        easing: Easing function ID.
    """

    value: float
    time: float
    easing: int


@dataclass
class RotationKeyFrame:
    """A rotation key frame in a camera spline.

    Attributes:
        value: Rotation vector (x, y, z).
        time: Key frame time.
        easing: Easing function ID.
    """

    value: tuple[float, float, float]
    time: float
    easing: int


@dataclass
class SplineInstruction:
    """A CameraInstruction::SplineInstruction.

    Attributes:
        total_time: Total duration of the spline.
        type: Spline type ID.
        curve: List of curve control points.
        progress_key_frames: Progress key frames.
        rotation_key_frames: Rotation key frames with Vec3 values.
        spline_identifier: Spline identifier string (v944+).
        load_from_json: Whether to load from JSON (v944+).
    """

    total_time: float
    type: int
    curve: list[tuple[float, float, float]] = field(default_factory=list)
    progress_key_frames: list[KeyFrame] = field(default_factory=list)
    rotation_key_frames: list[RotationKeyFrame] = field(default_factory=list)
    spline_identifier: str = ""
    load_from_json: bool = False


_DEFAULT_EASING = 0  # linear


class _SplineInstructionV898Type(Type["SplineInstruction"]):
    """v898 SplineInstruction: no easing, progress frames are Vec2 (value, time)."""

    def read(self, reader: PacketReader) -> SplineInstruction:
        total_time = FLOAT_LE.read(reader)  # totalTime
        type_ = BYTE.read(reader)  # type
        # curve
        curve: list[tuple[float, float, float]] = []
        for _ in range(UVAR_INT.read(reader)):
            curve.append(VEC3.read(reader))
        # progressKeyFrames (Vec2: X=value, Y=time, no easing)
        progress: list[KeyFrame] = []
        for _ in range(UVAR_INT.read(reader)):
            value = FLOAT_LE.read(reader)
            time = FLOAT_LE.read(reader)
            progress.append(KeyFrame(value=value, time=time, easing=_DEFAULT_EASING))
        # rotationOption (Vec3 value + float time, no easing)
        rotation: list[RotationKeyFrame] = []
        for _ in range(UVAR_INT.read(reader)):
            rot_value = VEC3.read(reader)
            rot_time = FLOAT_LE.read(reader)
            rotation.append(RotationKeyFrame(value=rot_value, time=rot_time, easing=_DEFAULT_EASING))
        return SplineInstruction(total_time, type_, curve, progress, rotation)

    def write(self, writer: PacketWriter, value: SplineInstruction) -> None:
        FLOAT_LE.write(writer, value.total_time)  # totalTime
        BYTE.write(writer, value.type)  # type
        # curve
        UVAR_INT.write(writer, len(value.curve))
        for point in value.curve:
            VEC3.write(writer, point)
        # progressKeyFrames (Vec2: value, time -- drop easing)
        UVAR_INT.write(writer, len(value.progress_key_frames))
        for kf in value.progress_key_frames:
            FLOAT_LE.write(writer, kf.value)
            FLOAT_LE.write(writer, kf.time)
        # rotationOption (Vec3 value + float time -- drop easing)
        UVAR_INT.write(writer, len(value.rotation_key_frames))
        for rkf in value.rotation_key_frames:
            VEC3.write(writer, rkf.value)
            FLOAT_LE.write(writer, rkf.time)


class _SplineInstructionV924Type(Type["SplineInstruction"]):
    """v924 SplineInstruction: base fields only."""

    def read(self, reader: PacketReader) -> SplineInstruction:
        total_time = FLOAT_LE.read(reader)  # totalTime
        type_ = BYTE.read(reader)  # type
        # curve
        curve: list[tuple[float, float, float]] = []
        for _ in range(UVAR_INT.read(reader)):
            curve.append(VEC3.read(reader))
        # progressKeyFrames
        progress: list[KeyFrame] = []
        for _ in range(UVAR_INT.read(reader)):
            progress.append(KeyFrame(
                value=FLOAT_LE.read(reader),
                time=FLOAT_LE.read(reader),
                easing=INT_LE.read(reader),
            ))
        # rotationOption
        rotation: list[RotationKeyFrame] = []
        for _ in range(UVAR_INT.read(reader)):
            rotation.append(RotationKeyFrame(
                value=VEC3.read(reader),
                time=FLOAT_LE.read(reader),
                easing=INT_LE.read(reader),
            ))
        return SplineInstruction(total_time, type_, curve, progress, rotation)

    def write(self, writer: PacketWriter, value: SplineInstruction) -> None:
        FLOAT_LE.write(writer, value.total_time)  # totalTime
        BYTE.write(writer, value.type)  # type
        # curve
        UVAR_INT.write(writer, len(value.curve))
        for point in value.curve:
            VEC3.write(writer, point)
        # progressKeyFrames
        UVAR_INT.write(writer, len(value.progress_key_frames))
        for kf in value.progress_key_frames:
            FLOAT_LE.write(writer, kf.value)
            FLOAT_LE.write(writer, kf.time)
            INT_LE.write(writer, kf.easing)
        # rotationOption
        UVAR_INT.write(writer, len(value.rotation_key_frames))
        for rkf in value.rotation_key_frames:
            VEC3.write(writer, rkf.value)
            FLOAT_LE.write(writer, rkf.time)
            INT_LE.write(writer, rkf.easing)


class _SplineInstructionV944Type(_SplineInstructionV924Type):
    """v944 SplineInstruction: base fields + splineIdentifier + loadFromJson."""

    def read(self, reader: PacketReader) -> SplineInstruction:
        spline = super().read(reader)
        spline.spline_identifier = STRING.read(reader)  # splineIdentifier
        spline.load_from_json = BOOL.read(reader)  # loadFromJson
        return spline

    def write(self, writer: PacketWriter, value: SplineInstruction) -> None:
        super().write(writer, value)
        STRING.write(writer, value.spline_identifier)  # splineIdentifier
        BOOL.write(writer, value.load_from_json)  # loadFromJson


SPLINE_INSTRUCTION_V898 = _SplineInstructionV898Type()
SPLINE_INSTRUCTION_V924 = _SplineInstructionV924Type()
SPLINE_INSTRUCTION_V944 = _SplineInstructionV944Type()
