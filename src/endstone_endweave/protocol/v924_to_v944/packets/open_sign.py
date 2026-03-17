from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class OpenSignPacket:
    """Packet 303 - clientbound."""
    x: int; y: int; z: int
    is_front: bool

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> OpenSignPacket:
        x, y, z = read_pos(reader)
        is_front = reader.read_bool()
        return cls(x, y, z, is_front)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        write_pos(writer, self.x, self.y, self.z)
        writer.write_bool(self.is_front)
