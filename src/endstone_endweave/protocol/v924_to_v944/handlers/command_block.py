"""Handler for CommandBlockUpdatePacket (78) - serverbound."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.protocol.v924_to_v944.block_position import convert_944_to_924


def rewrite_command_block_update(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Downscale v944 client -> v924 server: BlockPos -> NetworkBlockPosition."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    is_block = reader.read_bool()
    writer.write_bool(is_block)

    if not is_block:
        # If False: ActorRuntimeID (unsigned varlong) - no change
        runtime_id = reader.read_varlong()
        writer.write_varlong(runtime_id)
    else:
        # If True: block position needs conversion (BlockPos -> NetworkBlockPosition)
        convert_944_to_924(reader, writer)

        # Command block mode (unsigned varint)
        mode = reader.read_varint()
        writer.write_varint(mode)

        # Redstone mode (bool)
        writer.write_bool(reader.read_bool())

        # Is conditional (bool)
        writer.write_bool(reader.read_bool())

    # Remaining fields identical - copy verbatim
    writer.write_bytes(reader.read_remaining())

    return PacketTransformation(new_payload=writer.to_bytes())
