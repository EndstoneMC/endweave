from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class AnvilDamagePacket:
    """Packet 141 - serverbound."""
    damage: int
    x: int; y: int; z: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> AnvilDamagePacket:
        damage = reader.read_byte()
        x, y, z = read_pos(reader)
        return cls(damage, x, y, z)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_byte(self.damage)
        write_pos(writer, self.x, self.y, self.z)
