from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class SetSpawnPositionPacket:
    """Packet 43 - clientbound. Has 2 block positions."""
    spawn_type: int
    x: int; y: int; z: int
    dimension: int
    spawn_x: int; spawn_y: int; spawn_z: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> SetSpawnPositionPacket:
        spawn_type = reader.read_signed_varint()
        x, y, z = read_pos(reader)
        dimension = reader.read_signed_varint()
        spawn_x, spawn_y, spawn_z = read_pos(reader)
        return cls(spawn_type, x, y, z, dimension, spawn_x, spawn_y, spawn_z)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_signed_varint(self.spawn_type)
        write_pos(writer, self.x, self.y, self.z)
        writer.write_signed_varint(self.dimension)
        write_pos(writer, self.spawn_x, self.spawn_y, self.spawn_z)
