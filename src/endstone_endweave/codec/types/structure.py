"""Structure-related compound types (StructureSettings)."""

from dataclasses import dataclass

from endstone_endweave.codec.reader import PacketReader
from endstone_endweave.codec.writer import PacketWriter

from .primitives import (
    BLOCK_POS,
    BOOL,
    BYTE,
    FLOAT_LE,
    NETWORK_BLOCK_POS,
    STRING,
    UINT_LE,
    VAR_INT64,
    VEC3,
    Type,
)


@dataclass
class StructureSettings:
    """StructureSettings used by StructureBlockUpdate and StructureTemplateDataRequest.

    Attributes:
        palette_name: Structure palette name.
        ignore_entities: Whether to ignore entities.
        ignore_blocks: Whether to ignore blocks.
        allow_non_ticking_chunks: Whether to allow non-ticking chunks.
        size: Structure size as (x, y, z).
        offset: Structure offset as (x, y, z).
        last_edit_player: Unique ID of last editing player.
        rotation: Rotation enum value.
        mirror: Mirror enum value.
        animation_mode: Animation mode enum value.
        animation_seconds: Animation duration in seconds.
        integrity_value: Structure integrity (0.0 - 100.0).
        integrity_seed: Integrity seed.
        pivot: Rotation pivot point (x, y, z).
    """

    palette_name: str
    ignore_entities: bool
    ignore_blocks: bool
    allow_non_ticking_chunks: bool
    size: tuple[int, int, int]
    offset: tuple[int, int, int]
    last_edit_player: int
    rotation: int
    mirror: int
    animation_mode: int
    animation_seconds: float
    integrity_value: float
    integrity_seed: int
    pivot: tuple[float, float, float]


class _StructureSettingsV924Type(Type["StructureSettings"]):
    """v924 StructureSettings: Size/Offset encoded as NetworkBlockPos (uvarint Y)."""

    _pos_type: Type[tuple[int, int, int]] = NETWORK_BLOCK_POS

    def read(self, reader: PacketReader) -> StructureSettings:
        return StructureSettings(
            palette_name=STRING.read(reader),
            ignore_entities=BOOL.read(reader),
            ignore_blocks=BOOL.read(reader),
            allow_non_ticking_chunks=BOOL.read(reader),
            size=self._pos_type.read(reader),
            offset=self._pos_type.read(reader),
            last_edit_player=VAR_INT64.read(reader),
            rotation=BYTE.read(reader),
            mirror=BYTE.read(reader),
            animation_mode=BYTE.read(reader),
            animation_seconds=FLOAT_LE.read(reader),
            integrity_value=FLOAT_LE.read(reader),
            integrity_seed=UINT_LE.read(reader),
            pivot=VEC3.read(reader),
        )

    def write(self, writer: PacketWriter, value: StructureSettings) -> None:
        STRING.write(writer, value.palette_name)
        BOOL.write(writer, value.ignore_entities)
        BOOL.write(writer, value.ignore_blocks)
        BOOL.write(writer, value.allow_non_ticking_chunks)
        self._pos_type.write(writer, value.size)
        self._pos_type.write(writer, value.offset)
        VAR_INT64.write(writer, value.last_edit_player)
        BYTE.write(writer, value.rotation)
        BYTE.write(writer, value.mirror)
        BYTE.write(writer, value.animation_mode)
        FLOAT_LE.write(writer, value.animation_seconds)
        FLOAT_LE.write(writer, value.integrity_value)
        UINT_LE.write(writer, value.integrity_seed)
        VEC3.write(writer, value.pivot)


class _StructureSettingsV944Type(_StructureSettingsV924Type):
    """v944 StructureSettings: Size/Offset encoded as BlockPos (varint Y)."""

    _pos_type: Type[tuple[int, int, int]] = BLOCK_POS


STRUCTURE_SETTINGS_V924 = _StructureSettingsV924Type()
STRUCTURE_SETTINGS_V944 = _StructureSettingsV944Type()
