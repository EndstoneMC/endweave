from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class BlockEventPacket:
    """Packet 26 - clientbound."""
    x: int; y: int; z: int
    event_type: int; event_value: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> BlockEventPacket:
        x, y, z = read_pos(reader)
        event_type = reader.read_signed_varint()
        event_value = reader.read_signed_varint()
        return cls(x, y, z, event_type, event_value)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        write_pos(writer, self.x, self.y, self.z)
        writer.write_signed_varint(self.event_type)
        writer.write_signed_varint(self.event_value)
