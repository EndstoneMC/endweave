from __future__ import annotations

from dataclasses import dataclass

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.protocol.v924_to_v944.block_position import (
    PosReader,
    PosWriter,
)


@dataclass(frozen=True, slots=True)
class UpdateBlockSyncedPacket:
    """Packet 110 - clientbound."""
    x: int; y: int; z: int
    runtime_id: int; flags: int; layer: int
    actor_id: int; sync_message: int

    @classmethod
    def decode(cls, reader: PacketReader, read_pos: PosReader) -> UpdateBlockSyncedPacket:
        x, y, z = read_pos(reader)
        runtime_id = reader.read_varint()
        flags = reader.read_varint()
        layer = reader.read_varint()
        actor_id = reader.read_varlong()
        sync_message = reader.read_varlong()
        return cls(x, y, z, runtime_id, flags, layer, actor_id, sync_message)

    def encode(self, writer: PacketWriter, write_pos: PosWriter) -> None:
        write_pos(writer, self.x, self.y, self.z)
        writer.write_varint(self.runtime_id)
        writer.write_varint(self.flags)
        writer.write_varint(self.layer)
        writer.write_varlong(self.actor_id)
        writer.write_varlong(self.sync_message)
