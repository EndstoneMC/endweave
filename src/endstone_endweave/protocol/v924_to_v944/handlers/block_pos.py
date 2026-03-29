"""Packet handlers for NetworkBlockPos -> BlockPos changes (v924 -> v944).

Clientbound handlers read NetworkBlockPos (from v924 server) and write BlockPos (for v944 client).
Serverbound handlers read BlockPos (from v944 client) and write NetworkBlockPos (for v924 server).
"""

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    FLOAT_LE,
    INT_LE,
    INVENTORY_ACTION,
    ITEM_INSTANCE,
    NETWORK_BLOCK_POS,
    STRING,
    STRUCTURE_SETTINGS_V924,
    STRUCTURE_SETTINGS_V944,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    VEC3,
    ArrayType,
    ClientboundMapItemDataType,
    ComplexInventoryTransactionType,
    MapItemTrackedActorType,
    PacketWrapper,
)

# NoteBlockInstrument remapping (TileEvent)
_NOTE_BLOCK_EVENT = 0
_TRUMPET_INSERTION_POINT = 16
_TRUMPET_ID_SHIFT = 4


# ---------------------------------------------------------------------------
# Clientbound (server -> client): NetworkBlockPos -> BlockPos
# ---------------------------------------------------------------------------


def rewrite_first_net_block_to_block(wrapper: PacketWrapper) -> None:
    """Rewrite first-field NetworkBlockPos -> BlockPos.

    Used by: UpdateBlock (21), BlockActorData (56),
    UpdateBlockSynced (110), LecternUpdate (125), OpenSign (303).

    Args:
        wrapper: Packet wrapper positioned at the first field.
    """
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)


def rewrite_tile_event(wrapper: PacketWrapper) -> None:
    """BlockEventPacket (26): convert NetworkBlockPos -> BlockPos, remap NoteBlockInstrument.

    v944 inserted Trumpet variants at IDs 16-19, displacing Zombie..Piglin by +4.

    Args:
        wrapper: Packet wrapper for TileEvent.
    """
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Block Position
    event_type = wrapper.passthrough(VAR_INT)  # Event Type
    event_data = wrapper.read(VAR_INT)  # Event Value
    if event_type == _NOTE_BLOCK_EVENT and event_data >= _TRUMPET_INSERTION_POINT:
        event_data += _TRUMPET_ID_SHIFT
    wrapper.write(VAR_INT, event_data)


def rewrite_set_spawn_position(wrapper: PacketWrapper) -> None:
    """SetSpawnPosition (43): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for SetSpawnPosition.
    """
    wrapper.passthrough(VAR_INT)  # Spawn Position Type
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Block Position
    wrapper.passthrough(VAR_INT)  # Dimension type
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Spawn Block Pos


def rewrite_add_volume_entity(wrapper: PacketWrapper) -> None:
    """AddVolumeEntity (166): convert NetworkBlockPos -> BlockPos in bounds.

    Args:
        wrapper: Packet wrapper for AddVolumeEntity.
    """
    wrapper.passthrough(UVAR_INT)  # Entity Network Id
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Min Bounds
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Max Bounds


def rewrite_update_sub_chunk_blocks(wrapper: PacketWrapper) -> None:
    """UpdateSubChunkBlocks (172): convert NetworkBlockPos -> BlockPos in all entries.

    Args:
        wrapper: Packet wrapper for UpdateSubChunkBlocks.
    """
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Sub Chunk Block Position

    # Blocks Changed - Standards
    blocks_count = wrapper.passthrough(UVAR_INT)
    for _ in range(blocks_count):
        wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Pos
        wrapper.passthrough(UVAR_INT)  # Runtime Id
        wrapper.passthrough(UVAR_INT)  # Update Flags
        wrapper.passthrough(UVAR_INT64)  # Sync Message - Entity Unique ID
        wrapper.passthrough(UVAR_INT)  # Sync Message - Message

    # Blocks Changed - Extras
    extra_count = wrapper.passthrough(UVAR_INT)
    for _ in range(extra_count):
        wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Pos
        wrapper.passthrough(UVAR_INT)  # Runtime Id
        wrapper.passthrough(UVAR_INT)  # Update Flags
        wrapper.passthrough(UVAR_INT64)  # Sync Message - Entity Unique ID
        wrapper.passthrough(UVAR_INT)  # Sync Message - Message


def rewrite_play_sound(wrapper: PacketWrapper) -> None:
    """PlaySound (86): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for PlaySound.
    """
    wrapper.passthrough(STRING)  # Name
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Position


def rewrite_map_data(wrapper: PacketWrapper) -> None:
    """ClientboundMapItemData (67): convert tracked block object positions.

    Only converts when Type Flags has the Decoration bit (0x04)
    and the object Type is Block (1).

    Args:
        wrapper: Packet wrapper for ClientboundMapItemData.
    """
    wrapper.passthrough(VAR_INT64)  # Map ID
    types = wrapper.passthrough(UVAR_INT)  # Type Flags
    wrapper.passthrough(BYTE)  # Dimension
    wrapper.passthrough(BOOL)  # Is Locked Map?
    wrapper.passthrough(BLOCK_POS)  # Map Origin

    if types & ClientboundMapItemDataType.CREATION:
        wrapper.passthrough(ArrayType(VAR_INT64))  # Map ID List

    if types & (
        ClientboundMapItemDataType.CREATION
        | ClientboundMapItemDataType.DECORATION_UPDATE
        | ClientboundMapItemDataType.TEXTURE_UPDATE
    ):
        wrapper.passthrough(BYTE)  # Scale

    if types & ClientboundMapItemDataType.DECORATION_UPDATE:
        # Actor IDs
        obj_count = wrapper.passthrough(UVAR_INT)
        for _ in range(obj_count):
            obj_type = wrapper.passthrough(INT_LE)  # Type
            if obj_type == MapItemTrackedActorType.ENTITY:
                wrapper.passthrough(VAR_INT64)  # MapItemTrackedActor::UniqueId
            elif obj_type == MapItemTrackedActorType.BLOCK_ENTITY:
                wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Block Position


def rewrite_update_client_input_locks(wrapper: PacketWrapper) -> None:
    """UpdateClientInputLocks (196): strip trailing Server Pos removed in v944.

    Args:
        wrapper: Packet wrapper for UpdateClientInputLocks.
    """
    wrapper.passthrough(UVAR_INT)  # Input Lock ComponentData
    wrapper.read(FLOAT_LE)  # discard Server Pos.X
    wrapper.read(FLOAT_LE)  # discard Server Pos.Y
    wrapper.read(FLOAT_LE)  # discard Server Pos.Z


# ---------------------------------------------------------------------------
# Serverbound (client -> server): BlockPos -> NetworkBlockPos
# ---------------------------------------------------------------------------


def rewrite_first_block_to_net_block(wrapper: PacketWrapper) -> None:
    """Rewrite first-field BlockPos -> NetworkBlockPos.

    Used by: BlockActorData (56).

    Args:
        wrapper: Packet wrapper positioned at the first field.
    """
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)


def rewrite_inventory_transaction(wrapper: PacketWrapper) -> None:
    """Rewrite InventoryTransaction: convert BlockPos -> NetworkBlockPos in UseItem data.

    Args:
        wrapper: Packet wrapper for InventoryTransaction.
    """
    legacy_request_id = wrapper.passthrough(VAR_INT)  # Raw Id (32 bit signed)
    if legacy_request_id != 0:
        # Legacy Set Item Slots
        slot_count = wrapper.passthrough(UVAR_INT)
        for _ in range(slot_count):
            wrapper.passthrough(BYTE)  # Container Enum
            # Slot vector
            slots_len = wrapper.passthrough(UVAR_INT)
            for _ in range(slots_len):
                wrapper.passthrough(BYTE)  # Slot

    transaction_type = wrapper.passthrough(UVAR_INT)  # Transaction Type

    # InventoryActions
    action_count = wrapper.passthrough(UVAR_INT)
    for _ in range(action_count):
        wrapper.passthrough(INVENTORY_ACTION)

    if transaction_type != ComplexInventoryTransactionType.ITEM_USE_TRANSACTION:
        return  # passthrough remaining bytes unchanged

    # UseItemTransactionData
    wrapper.passthrough(UVAR_INT)  # ActionType
    wrapper.passthrough(UVAR_INT)  # TriggerType
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # BlockPosition
    wrapper.passthrough(VAR_INT)  # BlockFace
    wrapper.passthrough(VAR_INT)  # HotBarSlot
    wrapper.passthrough(ITEM_INSTANCE)  # HeldItem
    wrapper.passthrough(VEC3)  # Position
    wrapper.passthrough(VEC3)  # ClickedPosition
    wrapper.passthrough(UVAR_INT)  # BlockRuntimeID
    wrapper.passthrough(UVAR_INT)  # ClientPrediction
    wrapper.read(BYTE)  # ClientCooldownState (strip -- v924 doesn't have this)


def rewrite_player_action(wrapper: PacketWrapper) -> None:
    """PlayerAction (36): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for PlayerAction.
    """
    wrapper.passthrough(UVAR_INT64)  # Player Runtime ID
    wrapper.passthrough(VAR_INT)  # Action
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # Block Position
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # Result Pos


def rewrite_container_open(wrapper: PacketWrapper) -> None:
    """ContainerOpen (46): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for ContainerOpen.
    """
    wrapper.passthrough(BYTE)  # Container Id
    wrapper.passthrough(BYTE)  # Container Type
    wrapper.map(NETWORK_BLOCK_POS, BLOCK_POS)  # Position


def rewrite_structure_block_update(wrapper: PacketWrapper) -> None:
    """StructureBlockUpdate (90): convert BlockPos -> NetworkBlockPos in StructureSettings.

    Args:
        wrapper: Packet wrapper for StructureBlockUpdate.
    """
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # Block Position
    # StructureEditorData
    wrapper.passthrough(STRING)  # Name
    wrapper.passthrough(STRING)  # DataField
    wrapper.passthrough(BOOL)  # IncludePlayers
    wrapper.passthrough(BOOL)  # ShowBoundingBox
    wrapper.passthrough(VAR_INT)  # StructureBlockType
    wrapper.map(STRUCTURE_SETTINGS_V944, STRUCTURE_SETTINGS_V924)


def rewrite_command_block_update(wrapper: PacketWrapper) -> None:
    """CommandBlockUpdate (78): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for CommandBlockUpdate.
    """
    is_block = wrapper.passthrough(BOOL)  # Is Block?
    if is_block:
        wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # Block Position


def rewrite_structure_template_data_request(wrapper: PacketWrapper) -> None:
    """StructureTemplateDataRequest (132): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for StructureTemplateDataRequest.
    """
    wrapper.passthrough(STRING)  # Structure Name
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # Structure Position
    wrapper.map(STRUCTURE_SETTINGS_V944, STRUCTURE_SETTINGS_V924)


def rewrite_anvil_damage(wrapper: PacketWrapper) -> None:
    """AnvilDamage (141): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for AnvilDamage.
    """
    wrapper.passthrough(BYTE)  # Damage Amount
    wrapper.map(BLOCK_POS, NETWORK_BLOCK_POS)  # Block Position
