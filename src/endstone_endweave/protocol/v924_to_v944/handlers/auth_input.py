"""Handler for PlayerAuthInputPacket (144) - serverbound."""

from __future__ import annotations

from endstone_endweave.codec import PacketReader, PacketWriter
from endstone_endweave.session import PlayerSession
from endstone_endweave.protocol.base import PacketTransformation
from endstone_endweave.protocol.v924_to_v944.block_position import convert_944_to_924

# Bit index for PerformItemInteraction in the Input Data bitset
PERFORM_ITEM_INTERACTION_BIT = 23


def rewrite_auth_input(
    payload: bytes, session: PlayerSession
) -> PacketTransformation:
    """Downscale v944 -> v924: convert BlockPos to NetworkBlockPosition."""
    reader = PacketReader(payload)
    writer = PacketWriter()

    # 1. Player Rotation (Vec2 = 8 bytes)
    writer.write_bytes(reader.read_bytes(8))
    # 2. Position (Vec3 = 12 bytes)
    writer.write_bytes(reader.read_bytes(12))
    # 3. Move Vector (Vec2 = 8 bytes)
    writer.write_bytes(reader.read_bytes(8))
    # 4. Head Rotation (float = 4 bytes)
    writer.write_bytes(reader.read_bytes(4))
    # 5. Input Data (bitset<65> as unsigned varlong)
    input_data = reader.read_varlong()
    writer.write_varlong(input_data)
    # 6. Input Mode (unsigned varint)
    writer.write_varint(reader.read_varint())
    # 7. Play Mode (unsigned varint)
    writer.write_varint(reader.read_varint())
    # 8. New Interaction Model (unsigned varint)
    writer.write_varint(reader.read_varint())
    # 9. Interact Rotation (Vec2 = 8 bytes)
    writer.write_bytes(reader.read_bytes(8))
    # 10. Client Tick (unsigned varlong)
    writer.write_varlong(reader.read_varlong())
    # 11. Pos Delta (Vec3 = 12 bytes)
    writer.write_bytes(reader.read_bytes(12))

    has_item_interaction = bool(input_data & (1 << PERFORM_ITEM_INTERACTION_BIT))

    if not has_item_interaction:
        # No item transaction - copy rest verbatim
        writer.write_bytes(reader.read_remaining())
        return PacketTransformation(new_payload=writer.to_bytes())

    # 12. PackedItemUseLegacyInventoryTransaction
    # Legacy Request Id (signed varint)
    legacy_request_id = reader.read_signed_varint()
    writer.write_signed_varint(legacy_request_id)

    if legacy_request_id != 0:
        # Legacy Set Item Slots list
        slot_count = reader.read_varint()
        writer.write_varint(slot_count)
        for _ in range(slot_count):
            # container id (byte) + slot list
            writer.write_byte(reader.read_byte())
            inner_count = reader.read_varint()
            writer.write_varint(inner_count)
            for _ in range(inner_count):
                writer.write_byte(reader.read_byte())

    # Actions list
    action_count = reader.read_varint()
    writer.write_varint(action_count)
    for _ in range(action_count):
        # Each action: source type (unsigned varint), then type-dependent data
        # This is complex - copy action data raw since it doesn't contain block positions
        # Source type
        source_type = reader.read_varint()
        writer.write_varint(source_type)
        # All InventorySource types have the same tail: container id (varint), slot (varint), item instances
        # But item instances are complex (NBT data). We can't parse them generically.
        # Bail out: copy rest of payload verbatim from here. The block position
        # in the transaction data comes AFTER actions, but we can't determine
        # action boundaries without full item stack deserialization.
        writer.write_bytes(reader.read_remaining())
        return PacketTransformation(new_payload=writer.to_bytes())

    # If we got here, action_count was 0. Continue to transaction data.
    # Action Type (unsigned varint)
    action_type = reader.read_varint()
    writer.write_varint(action_type)

    # Action types: 0=ItemUse, 1=ItemUseOnEntity, 2=ItemRelease
    # Only ItemUse (0) has a block position after trigger type
    if action_type == 0:
        # Trigger Type (unsigned varint)
        writer.write_varint(reader.read_varint())
        # Position - THE CONVERSION (BlockPos -> NetworkBlockPosition)
        convert_944_to_924(reader, writer)

    # Copy rest verbatim
    writer.write_bytes(reader.read_remaining())

    return PacketTransformation(new_payload=writer.to_bytes())
