from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class BlockActorDataPacket:
    """Packet 56 - clientbound. NBT follows the position - copied verbatim."""
    x: int; y: int; z: int
    trailing: bytes

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> BlockActorDataPacket:
        x, y, z = read_pos(reader)
        trailing = reader.read_remaining()
        return cls(x, y, z, trailing)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        write_pos(writer, self.x, self.y, self.z)
        writer.write_bytes(self.trailing)
