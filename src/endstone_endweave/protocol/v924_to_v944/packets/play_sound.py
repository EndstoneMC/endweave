from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class PlaySoundPacket:
    """Packet 86 - clientbound."""
    name: str
    x: int; y: int; z: int
    volume: float; pitch: float

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> PlaySoundPacket:
        name = reader.read_string()
        x, y, z = read_pos(reader)
        volume = reader.read_float_le()
        pitch = reader.read_float_le()
        return cls(name, x, y, z, volume, pitch)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_string(self.name)
        write_pos(writer, self.x, self.y, self.z)
        writer.write_float_le(self.volume)
        writer.write_float_le(self.pitch)
