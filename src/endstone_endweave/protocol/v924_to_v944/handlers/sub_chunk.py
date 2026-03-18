"""Handler for UpdateSubChunkBlocksPacket (172) - clientbound."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.protocol.v924_to_v944.block_position import convert_924_to_944


def rewrite_sub_chunk_blocks(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Upscale v924 server -> v944 client."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    # SubChunkPos: 3x signed varint (no change)
    for _ in range(3):
        writer.write_signed_varint(reader.read_signed_varint())

    # Process both arrays (standards, then extras)
    for _ in range(2):
        count = reader.read_varint()
        writer.write_varint(count)

        for _ in range(count):
            # Convert NetworkBlockPosition -> BlockPos
            convert_924_to_944(reader, writer)

            # runtime_id (unsigned varint)
            writer.write_varint(reader.read_varint())
            # update_flags (unsigned varint)
            writer.write_varint(reader.read_varint())
            # entity_id (varlong)
            writer.write_varlong(reader.read_varlong())
            # message_id (unsigned varint)
            writer.write_varint(reader.read_varint())

    return PacketTransformation(new_payload=writer.to_bytes())
