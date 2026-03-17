from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class StructureSettings:
    """Nested inside StructureTemplateDataRequestPacket."""
    palette_name: str
    ignore_entities: bool; ignore_blocks: bool; ignore_jigsaw_blocks: bool
    size_x: int; size_y: int; size_z: int
    offset_x: int; offset_y: int; offset_z: int
    last_edited_by: int
    rotation: int; mirror: int; animation_mode: int; animation_seconds: float
    integrity: float; seed: int
    pivot_x: float; pivot_y: float; pivot_z: float

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> StructureSettings:
        palette_name = reader.read_string()
        ignore_entities = reader.read_bool()
        ignore_blocks = reader.read_bool()
        ignore_jigsaw_blocks = reader.read_bool()
        size_x, size_y, size_z = read_pos(reader)
        offset_x, offset_y, offset_z = read_pos(reader)
        last_edited_by = reader.read_varlong()
        rotation = reader.read_byte()
        mirror = reader.read_byte()
        animation_mode = reader.read_byte()
        animation_seconds = reader.read_float_le()
        integrity = reader.read_float_le()
        seed = reader.read_uint_le()
        pivot_x = reader.read_float_le()
        pivot_y = reader.read_float_le()
        pivot_z = reader.read_float_le()
        return cls(
            palette_name, ignore_entities, ignore_blocks, ignore_jigsaw_blocks,
            size_x, size_y, size_z, offset_x, offset_y, offset_z,
            last_edited_by, rotation, mirror, animation_mode, animation_seconds,
            integrity, seed, pivot_x, pivot_y, pivot_z,
        )

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_string(self.palette_name)
        writer.write_bool(self.ignore_entities)
        writer.write_bool(self.ignore_blocks)
        writer.write_bool(self.ignore_jigsaw_blocks)
        write_pos(writer, self.size_x, self.size_y, self.size_z)
        write_pos(writer, self.offset_x, self.offset_y, self.offset_z)
        writer.write_varlong(self.last_edited_by)
        writer.write_byte(self.rotation)
        writer.write_byte(self.mirror)
        writer.write_byte(self.animation_mode)
        writer.write_float_le(self.animation_seconds)
        writer.write_float_le(self.integrity)
        writer.write_uint_le(self.seed)
        writer.write_float_le(self.pivot_x)
        writer.write_float_le(self.pivot_y)
        writer.write_float_le(self.pivot_z)


@dataclass(frozen=True, slots=True)
class StructureTemplateDataRequestPacket:
    """Packet 132 - serverbound. 3 positions: top-level + StructureSettings.size + offset."""
    name: str
    x: int; y: int; z: int
    settings: StructureSettings
    request_type: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> StructureTemplateDataRequestPacket:
        name = reader.read_string()
        x, y, z = read_pos(reader)
        settings = StructureSettings.decode(reader, read_pos)
        request_type = reader.read_byte()
        return cls(name, x, y, z, settings, request_type)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_string(self.name)
        write_pos(writer, self.x, self.y, self.z)
        self.settings.encode(writer, write_pos)
        writer.write_byte(self.request_type)
