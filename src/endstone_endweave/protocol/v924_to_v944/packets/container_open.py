from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class ContainerOpenPacket:
    """Packet 46 - clientbound."""
    container_id: int; container_type: int
    x: int; y: int; z: int
    target_actor_id: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> ContainerOpenPacket:
        container_id = reader.read_byte()
        container_type = reader.read_byte()
        x, y, z = read_pos(reader)
        target_actor_id = reader.read_varlong()
        return cls(container_id, container_type, x, y, z, target_actor_id)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        writer.write_byte(self.container_id)
        writer.write_byte(self.container_type)
        write_pos(writer, self.x, self.y, self.z)
        writer.write_varlong(self.target_actor_id)
