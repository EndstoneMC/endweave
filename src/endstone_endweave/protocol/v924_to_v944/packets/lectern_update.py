from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class LecternUpdatePacket:
    """Packet 125 - serverbound."""
    page: int; total_pages: int
    x: int; y: int; z: int
    trailing: bytes

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> LecternUpdatePacket:
        page = reader.read_byte()
        total_pages = reader.read_byte()
        x, y, z = read_pos(reader)
        trailing = reader.read_remaining()
        return cls(page, total_pages, x, y, z, trailing)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_byte(self.page)
        writer.write_byte(self.total_pages)
        write_pos(writer, self.x, self.y, self.z)
        writer.write_bytes(self.trailing)
