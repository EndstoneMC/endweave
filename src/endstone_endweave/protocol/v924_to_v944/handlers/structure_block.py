"""Handler for StructureBlockUpdatePacket (90) - serverbound."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.player_state import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.protocol.v924_to_v944.block_position import convert_944_to_924


def rewrite_structure_block_update(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Downscale v944 client -> v924 server: BlockPos -> NetworkBlockPosition."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    convert_944_to_924(reader, writer)
    writer.write_bytes(reader.read_remaining())

    return PacketTransformation(new_payload=writer.to_bytes())
