"""Shared rewriter helpers for cross-version packet field conversion.

These helpers are used by multiple version-specific protocol modules.
"""

from collections.abc import Callable, Mapping
from typing import Any

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    FLOAT_LE,
    ITEM_INSTANCE,
    NAMED_COMPOUND_TAG,
    NETWORK_BLOCK_POS,
    SHORT_LE,
    STRING,
    UINT_LE,
    UVAR_INT,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
    Type,
)


def net_to_block(wrapper: PacketWrapper) -> None:
    """Read NetworkBlockPos (v924) and write BlockPos (v944).

    Args:
        wrapper: Packet wrapper positioned at a NetworkBlockPos field.
    """
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))


def block_to_net(wrapper: PacketWrapper) -> None:
    """Read BlockPos (v944) and write NetworkBlockPos (v924).

    Args:
        wrapper: Packet wrapper positioned at a BlockPos field.
    """
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))


def passthrough_inventory_action(wrapper: PacketWrapper) -> None:
    """Passthrough a single InventoryAction entry.

    Args:
        wrapper: Packet wrapper positioned at an InventoryAction entry.
    """
    source_type = wrapper.passthrough(UVAR_INT)  # SourceType
    if source_type in (
        0,  # ContainerInventory
        99999,  # NonImplementedFeatureTODO
    ):
        wrapper.passthrough(VAR_INT)  # WindowID
    elif source_type == 2:  # WorldInteraction
        wrapper.passthrough(UVAR_INT)  # SourceFlags
    # GlobalInventory(1), CreativeInventory(3), InvalidInventory(0xFFFFFFFF): no extra fields
    wrapper.passthrough(UVAR_INT)  # InventorySlot
    wrapper.passthrough(ITEM_INSTANCE)  # OldItem
    wrapper.passthrough(ITEM_INSTANCE)  # NewItem


def passthrough_structure_settings(wrapper: PacketWrapper) -> None:
    """Passthrough StructureSettings, converting BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper positioned at a StructureSettings block.
    """
    wrapper.passthrough(STRING)  # Structure Palette Name
    wrapper.passthrough(BOOL)  # Should ignore entities?
    wrapper.passthrough(BOOL)  # Should ignore blocks?
    wrapper.passthrough(BOOL)  # Should Allow Non Ticking Player and Ticking Area Chunks
    block_to_net(wrapper)  # Structure Size
    block_to_net(wrapper)  # Structure Offset
    wrapper.passthrough(VAR_INT64)  # Last Edit Player
    wrapper.passthrough(BYTE)  # Rotation
    wrapper.passthrough(BYTE)  # Mirror
    wrapper.passthrough(BYTE)  # Animation Mode
    wrapper.passthrough(FLOAT_LE)  # Animation Seconds
    wrapper.passthrough(FLOAT_LE)  # Integrity Value
    wrapper.passthrough(UINT_LE)  # Integrity Seed
    wrapper.passthrough(FLOAT_LE)  # Rotation Pivot.X
    wrapper.passthrough(FLOAT_LE)  # Rotation Pivot.Y
    wrapper.passthrough(FLOAT_LE)  # Rotation Pivot.Z


# ---------------------------------------------------------------------------
# ActorData (entity metadata) helpers
# ---------------------------------------------------------------------------

# ActorData value type IDs -> how to passthrough each
_ACTOR_DATA_TYPES: dict[int, Type[Any]] = {
    0: BYTE,  # Byte
    1: SHORT_LE,  # Short
    2: VAR_INT,  # Int
    3: FLOAT_LE,  # Float
    4: STRING,  # String
    5: NAMED_COMPOUND_TAG,  # CompoundTag (Bedrock uses named NBT in ActorData)
    # 6 = BlockPos (3x varint), 7 = Int64, 8 = Vec3 (3x float) handled inline
}


def _passthrough_actor_data_value(wrapper: PacketWrapper, type_id: int) -> None:
    """Passthrough a single ActorData value based on its type ID.

    Args:
        wrapper: Packet wrapper positioned at the value.
        type_id: ActorData type ID (0-8).
    """
    if type_id in _ACTOR_DATA_TYPES:
        wrapper.passthrough(_ACTOR_DATA_TYPES[type_id])
    elif type_id == 6:  # BlockPos (3x varint)
        wrapper.passthrough(VAR_INT)
        wrapper.passthrough(VAR_INT)
        wrapper.passthrough(VAR_INT)
    elif type_id == 7:  # Int64
        wrapper.passthrough(VAR_INT64)
    elif type_id == 8:  # Vec3 (3x float)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)
        wrapper.passthrough(FLOAT_LE)


def passthrough_actor_data(
    wrapper: PacketWrapper,
    int_remappers: Mapping[int, Callable[[int], int]] | None = None,
) -> None:
    """Passthrough ActorData entries, optionally remapping Int (type 2) values.

    Args:
        wrapper: Packet wrapper positioned at the start of ActorData.
        int_remappers: Optional mapping of ActorData key -> remap function
            for Int-typed values. The function receives the original int
            and returns the remapped int.
    """
    count = wrapper.passthrough(UVAR_INT)
    for _ in range(count):
        key = wrapper.passthrough(UVAR_INT)
        type_id = wrapper.passthrough(UVAR_INT)

        if int_remappers and key in int_remappers and type_id in (2, 7):
            # Type 2 = Int (varint32), Type 7 = Int64 (varint64)
            field = VAR_INT if type_id == 2 else VAR_INT64
            value = wrapper.read(field)
            wrapper.write(field, int_remappers[key](value))
        else:
            _passthrough_actor_data_value(wrapper, type_id)
