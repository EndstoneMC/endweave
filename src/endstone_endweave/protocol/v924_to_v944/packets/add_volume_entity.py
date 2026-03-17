from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.codec.nbt import skip_nbt_compound
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class AddVolumeEntityPacket:
    """Packet 166 - clientbound. NBT before positions.

    Layout: runtime_id, nbt_compound, pos1, pos2, dimension, engine_version, definition_name
    """
    runtime_id: int
    nbt_bytes: bytes
    x1: int; y1: int; z1: int
    x2: int; y2: int; z2: int
    dimension: int; engine_version: str; definition_name: str

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> AddVolumeEntityPacket:
        runtime_id = reader.read_varint()
        nbt_start = reader.position
        skip_nbt_compound(reader)
        nbt_bytes = reader.slice_from(nbt_start)
        x1, y1, z1 = read_pos(reader)
        x2, y2, z2 = read_pos(reader)
        dimension = reader.read_signed_varint()
        engine_version = reader.read_string()
        definition_name = reader.read_string()
        return cls(runtime_id, nbt_bytes, x1, y1, z1, x2, y2, z2, dimension, engine_version, definition_name)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_varint(self.runtime_id)
        writer.write_bytes(self.nbt_bytes)
        write_pos(writer, self.x1, self.y1, self.z1)
        write_pos(writer, self.x2, self.y2, self.z2)
        writer.write_signed_varint(self.dimension)
        writer.write_string(self.engine_version)
        writer.write_string(self.definition_name)
