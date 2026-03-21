"""Shared rewriter helpers for cross-version packet field conversion.

These helpers are used by multiple version-specific protocol modules.
"""

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    FLOAT_LE,
    ITEM_INSTANCE,
    NETWORK_BLOCK_POS,
    STRING,
    UINT_LE,
    UVAR_INT,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
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

    Layout: string PaletteName, bool IgnoreEntities, bool IgnoreBlocks,
    bool AllowNonTickingChunks, BlockPos Size, BlockPos Offset,
    varint64 LastEditingPlayerUniqueID, byte Rotation, byte Mirror,
    byte AnimationMode, float AnimationSeconds, float IntegrityValue,
    uint32 IntegritySeed, Vec3 RotationPivot.

    Args:
        wrapper: Packet wrapper positioned at a StructureSettings block.
    """
    wrapper.passthrough(STRING)  # PaletteName
    wrapper.passthrough(BOOL)  # IgnoreEntities
    wrapper.passthrough(BOOL)  # IgnoreBlocks
    wrapper.passthrough(BOOL)  # AllowNonTickingChunks
    block_to_net(wrapper)  # Size
    block_to_net(wrapper)  # Offset
    wrapper.passthrough(VAR_INT64)  # LastEditingPlayerUniqueID
    wrapper.passthrough(BYTE)  # Rotation
    wrapper.passthrough(BYTE)  # Mirror
    wrapper.passthrough(BYTE)  # AnimationMode
    wrapper.passthrough(FLOAT_LE)  # AnimationSeconds
    wrapper.passthrough(FLOAT_LE)  # IntegrityValue
    wrapper.passthrough(UINT_LE)  # IntegritySeed
    wrapper.passthrough(FLOAT_LE)  # RotationPivot.X
    wrapper.passthrough(FLOAT_LE)  # RotationPivot.Y
    wrapper.passthrough(FLOAT_LE)  # RotationPivot.Z
