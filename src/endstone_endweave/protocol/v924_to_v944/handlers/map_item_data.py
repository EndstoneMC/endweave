"""Handler for ClientboundMapItemDataPacket (67) - clientbound."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.protocol.v924_to_v944.block_position import convert_924_to_944

# Bit flags for MapItemDataPacket type flags
_CREATION_BIT = 0x04
_DECORATION_BIT = 0x02
_TEXTURE_BIT = 0x08


def rewrite_map_item_data(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Upscale v924 -> v944: convert NetworkBlockPosition to BlockPos in tracked actor IDs."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    # 1. Map ID (ActorUniqueID = unsigned varlong)
    writer.write_varlong(reader.read_varlong())
    # 2. Type Flags (unsigned varint)
    type_flags = reader.read_varint()
    writer.write_varint(type_flags)
    # 3. Dimension (byte)
    writer.write_byte(reader.read_byte())
    # 4. Is Locked Map? (bool)
    writer.write_bool(reader.read_bool())
    # 5. Map Origin (BlockPos - already BlockPos in v924, no conversion)
    for _ in range(3):
        writer.write_signed_varint(reader.read_signed_varint())

    has_creation = bool(type_flags & _CREATION_BIT)
    has_decoration = bool(type_flags & _DECORATION_BIT)
    has_texture = bool(type_flags & _TEXTURE_BIT)

    # 6. [if Creation]: Map ID List
    if has_creation:
        id_count = reader.read_varint()
        writer.write_varint(id_count)
        for _ in range(id_count):
            writer.write_varlong(reader.read_varlong())

    # 7. [if Decoration|Texture|Creation]: Scale (byte)
    if has_decoration or has_texture or has_creation:
        writer.write_byte(reader.read_byte())

    # 8. [if Decoration]: Actor IDs list - contains block positions to convert
    if has_decoration:
        actor_id_count = reader.read_varint()
        writer.write_varint(actor_id_count)
        for _ in range(actor_id_count):
            # MapItemTrackedActor::UniqueId
            actor_type = reader.read_int_le()
            writer.write_int_le(actor_type)
            if actor_type == 0:
                # Entity: ActorUniqueID (unsigned varlong) - no conversion
                writer.write_varlong(reader.read_varlong())
            elif actor_type == 1:
                # BlockEntity: NetworkBlockPosition -> BlockPos
                convert_924_to_944(reader, writer)
            # else: unknown type, no additional data

    # Copy all remaining bytes verbatim (Decoration List + Texture section)
    writer.write_bytes(reader.read_remaining())

    return PacketTransformation(new_payload=writer.to_bytes())
