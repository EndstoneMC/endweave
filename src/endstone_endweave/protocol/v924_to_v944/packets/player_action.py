from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class PlayerActionPacket:
    """Packet 36 - serverbound. Has 2 block positions."""
    runtime_id: int; action: int
    x: int; y: int; z: int
    result_x: int; result_y: int; result_z: int
    face: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> PlayerActionPacket:
        runtime_id = reader.read_varlong()
        action = reader.read_signed_varint()
        x, y, z = read_pos(reader)
        result_x, result_y, result_z = read_pos(reader)
        face = reader.read_signed_varint()
        return cls(runtime_id, action, x, y, z, result_x, result_y, result_z, face)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_varlong(self.runtime_id)
        writer.write_signed_varint(self.action)
        write_pos(writer, self.x, self.y, self.z)
        write_pos(writer, self.result_x, self.result_y, self.result_z)
        writer.write_signed_varint(self.face)
