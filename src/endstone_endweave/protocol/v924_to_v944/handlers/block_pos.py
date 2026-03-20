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
    ITEM_INSTANCE,
    STRING,
    NETWORK_BLOCK_POS,
    UINT_LE,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)

# NoteBlockInstrument remapping constants (TileEvent)
_NOTE_BLOCK_EVENT = 0
_TRUMPET_INSERTION_POINT = 16
_TRUMPET_ID_SHIFT = 4


def _net_to_block(wrapper: PacketWrapper) -> None:
    """Read NetworkBlockPos (v924) and write BlockPos (v944).

    Args:
        wrapper: Packet wrapper positioned at a NetworkBlockPos field.
    """
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))


def _block_to_net(wrapper: PacketWrapper) -> None:
    """Read BlockPos (v944) and write NetworkBlockPos (v924).

    Args:
        wrapper: Packet wrapper positioned at a BlockPos field.
    """
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))


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
    _net_to_block(wrapper)


def rewrite_tile_event(wrapper: PacketWrapper) -> None:
    """TileEvent (26): Position, EventType, EventData.

    Converts NetworkBlockPos -> BlockPos, and remaps NoteBlockInstrument IDs.
    v944 inserted Trumpet variants at IDs 16-19, displacing Zombie..Piglin by +4.

    Args:
        wrapper: Packet wrapper for TileEvent.
    """
    _net_to_block(wrapper)  # Position
    event_type = wrapper.passthrough(VAR_INT)  # EventType
    event_data = wrapper.read(VAR_INT)  # EventData
    if event_type == _NOTE_BLOCK_EVENT and event_data >= _TRUMPET_INSERTION_POINT:
        event_data += _TRUMPET_ID_SHIFT
    wrapper.write(VAR_INT, event_data)


def rewrite_set_spawn_position(wrapper: PacketWrapper) -> None:
    """SetSpawnPosition (43): spawnType, Position, dimension, SpawnPosition.

    Args:
        wrapper: Packet wrapper for SetSpawnPosition.
    """
    wrapper.passthrough(VAR_INT)  # spawnType
    _net_to_block(wrapper)  # Position
    wrapper.passthrough(VAR_INT)  # dimension
    _net_to_block(wrapper)  # SpawnPosition


def rewrite_add_volume_entity(wrapper: PacketWrapper) -> None:
    """AddVolumeEntity (166): RuntimeID, Bounds[0], Bounds[1], then rest.

    Args:
        wrapper: Packet wrapper for AddVolumeEntity.
    """
    wrapper.passthrough(UVAR_INT)  # RuntimeID
    _net_to_block(wrapper)  # MinBound
    _net_to_block(wrapper)  # MaxBound


def rewrite_update_sub_chunk_blocks(wrapper: PacketWrapper) -> None:
    """UpdateSubChunkBlocks (172): Position, then Blocks/Extra slices.

    Each slice is uvarint count + BlockChangeEntry[].
    BlockChangeEntry: BlockPos, uvarint blockRuntimeID, uvarint flags,
    uvarint64 syncedUpdateEntityUniqueID, uvarint syncedUpdateType.

    Args:
        wrapper: Packet wrapper for UpdateSubChunkBlocks.
    """
    _net_to_block(wrapper)  # SubChunk position

    # Blocks slice
    blocks_count = wrapper.passthrough(UVAR_INT)
    for _ in range(blocks_count):
        _net_to_block(wrapper)  # BlockPos
        wrapper.passthrough(UVAR_INT)  # blockRuntimeID
        wrapper.passthrough(UVAR_INT)  # flags
        wrapper.passthrough(UVAR_INT64)  # syncedUpdateEntityUniqueID
        wrapper.passthrough(UVAR_INT)  # syncedUpdateType

    # Extra slice
    extra_count = wrapper.passthrough(UVAR_INT)
    for _ in range(extra_count):
        _net_to_block(wrapper)  # BlockPos
        wrapper.passthrough(UVAR_INT)  # blockRuntimeID
        wrapper.passthrough(UVAR_INT)  # flags
        wrapper.passthrough(UVAR_INT64)  # syncedUpdateEntityUniqueID
        wrapper.passthrough(UVAR_INT)  # syncedUpdateType


def rewrite_play_sound(wrapper: PacketWrapper) -> None:
    """PlaySound (86): Name, Position, Volume, Pitch.

    Args:
        wrapper: Packet wrapper for PlaySound.
    """
    wrapper.passthrough(STRING)  # Name
    _net_to_block(wrapper)  # Position


def rewrite_map_data(wrapper: PacketWrapper) -> None:
    """ClientBoundMapItemData (67): tracked block objects use UBlockPos in v924.

    Only the MapTrackedObject.BlockPosition field needs conversion, and only
    when UpdateFlags has the Decoration bit (0x04) and the object Type is Block (1).

    Args:
        wrapper: Packet wrapper for ClientBoundMapItemData.
    """
    wrapper.passthrough(VAR_INT64)  # MapID (varint64)
    types = wrapper.passthrough(UVAR_INT)  # UpdateFlags
    wrapper.passthrough(BYTE)  # Dimension
    wrapper.passthrough(BOOL)  # LockedMap
    wrapper.passthrough(BLOCK_POS)  # Origin (already signed varint Y)

    TYPE_TEXTURE_UPDATE = 0x02
    TYPE_DECORATION_UPDATE = 0x04
    TYPE_CREATION = 0x08

    if types & TYPE_CREATION:
        # mMapIds
        count = wrapper.passthrough(UVAR_INT)
        for _ in range(count):
            wrapper.passthrough(VAR_INT64)

    if types & (TYPE_CREATION | TYPE_DECORATION_UPDATE | TYPE_TEXTURE_UPDATE):
        wrapper.passthrough(BYTE)  # Scale

    if types & TYPE_DECORATION_UPDATE:
        # TrackedObjects
        obj_count = wrapper.passthrough(UVAR_INT)
        for _ in range(obj_count):
            obj_type = wrapper.passthrough(INT_LE)  # Type (int32)
            if obj_type == 0:  # Entity
                wrapper.passthrough(VAR_INT64)  # EntityUniqueID
            elif obj_type == 1:  # Block
                _net_to_block(wrapper)  # BlockPosition


def rewrite_update_client_input_locks(wrapper: PacketWrapper) -> None:
    """UpdateClientInputLocks (196): v944 removed trailing Vec3 (12 bytes).

    v924: varuint32 Locks + Vec3 (3x float32)
    v944: varuint32 Locks only

    Args:
        wrapper: Packet wrapper for UpdateClientInputLocks.
    """
    wrapper.passthrough(UVAR_INT)  # Locks
    wrapper.read(FLOAT_LE)  # discard Position.X
    wrapper.read(FLOAT_LE)  # discard Position.Y
    wrapper.read(FLOAT_LE)  # discard Position.Z


def rewrite_camera_spline(wrapper: PacketWrapper) -> None:
    """CameraSpline (338): v944 added optional SplineIdentifier + optional LoadFromJson.

    Append false + false (both optionals absent).

    Args:
        wrapper: Packet wrapper for CameraSpline.
    """
    wrapper.passthrough_all()
    wrapper.write(BOOL, False)  # has SplineIdentifier
    wrapper.write(BOOL, False)  # has LoadFromJson


# ---------------------------------------------------------------------------
# Serverbound (client -> server): BlockPos -> NetworkBlockPos
# ---------------------------------------------------------------------------


def _passthrough_inventory_action(wrapper: PacketWrapper) -> None:
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


def rewrite_inventory_transaction(wrapper: PacketWrapper) -> None:
    """Rewrite InventoryTransaction: convert BlockPos -> NetworkBlockPos in UseItem data.

    Args:
        wrapper: Packet wrapper for InventoryTransaction.
    """
    legacy_request_id = wrapper.passthrough(VAR_INT)
    if legacy_request_id != 0:
        # LegacySetItemSlots: uvarint count + [byte ContainerID + ByteSlice Slots]
        slot_count = wrapper.passthrough(UVAR_INT)
        for _ in range(slot_count):
            wrapper.passthrough(BYTE)  # ContainerID
            # Slots: uvarint length + bytes
            slots_len = wrapper.passthrough(UVAR_INT)
            for _ in range(slots_len):
                wrapper.passthrough(BYTE)

    transaction_type = wrapper.passthrough(UVAR_INT)  # TransactionDataType

    # InventoryActions
    action_count = wrapper.passthrough(UVAR_INT)
    for _ in range(action_count):
        _passthrough_inventory_action(wrapper)

    if transaction_type != 2:  # Not UseItem
        return  # passthrough remaining bytes unchanged

    # UseItemTransactionData
    wrapper.passthrough(UVAR_INT)  # ActionType
    wrapper.passthrough(UVAR_INT)  # TriggerType
    _block_to_net(wrapper)  # BlockPosition
    wrapper.passthrough(VAR_INT)  # BlockFace
    wrapper.passthrough(VAR_INT)  # HotBarSlot
    wrapper.passthrough(ITEM_INSTANCE)  # HeldItem
    wrapper.passthrough(FLOAT_LE)  # Position.X
    wrapper.passthrough(FLOAT_LE)  # Position.Y
    wrapper.passthrough(FLOAT_LE)  # Position.Z
    wrapper.passthrough(FLOAT_LE)  # ClickedPosition.X
    wrapper.passthrough(FLOAT_LE)  # ClickedPosition.Y
    wrapper.passthrough(FLOAT_LE)  # ClickedPosition.Z
    wrapper.passthrough(UVAR_INT)  # BlockRuntimeID
    wrapper.passthrough(UVAR_INT)  # ClientPrediction
    wrapper.read(BYTE)  # ClientCooldownState (strip -- v924 doesn't have this)


def rewrite_player_action(wrapper: PacketWrapper) -> None:
    """PlayerAction (36): entityRuntimeID, actionType, BlockPosition, ResultPosition, face.

    Args:
        wrapper: Packet wrapper for PlayerAction.
    """
    wrapper.passthrough(UVAR_INT64)  # entityRuntimeID
    wrapper.passthrough(VAR_INT)  # actionType
    _block_to_net(wrapper)  # BlockPosition
    _block_to_net(wrapper)  # ResultPosition


def rewrite_container_open(wrapper: PacketWrapper) -> None:
    """ContainerOpen (46): windowID, type, ContainerPosition, entityUniqueID.

    Args:
        wrapper: Packet wrapper for ContainerOpen.
    """
    wrapper.passthrough(BYTE)  # windowID
    wrapper.passthrough(BYTE)  # type
    _block_to_net(wrapper)  # ContainerPosition


def _rewrite_structure_settings(wrapper: PacketWrapper) -> None:
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
    _block_to_net(wrapper)  # Size
    _block_to_net(wrapper)  # Offset
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


def rewrite_structure_block_update(wrapper: PacketWrapper) -> None:
    """StructureBlockUpdate (90): Position, StructureEditorData, trigger, waterlogged.

    StructureEditorData: string Name, string DataField, bool IncludePlayers,
    bool ShowBoundingBox, varint StructureBlockType, StructureSettings, varint RedstoneSaveMode.

    Args:
        wrapper: Packet wrapper for StructureBlockUpdate.
    """
    _block_to_net(wrapper)  # Block Position
    # StructureEditorData
    wrapper.passthrough(STRING)  # Name
    wrapper.passthrough(STRING)  # DataField
    wrapper.passthrough(BOOL)  # IncludePlayers
    wrapper.passthrough(BOOL)  # ShowBoundingBox
    wrapper.passthrough(VAR_INT)  # StructureBlockType
    _rewrite_structure_settings(wrapper)
    wrapper.passthrough(VAR_INT)  # RedstoneSaveMode


def rewrite_command_block_update(wrapper: PacketWrapper) -> None:
    """CommandBlockUpdate (78): IsBlock (bool), then if true: Position first.

    Args:
        wrapper: Packet wrapper for CommandBlockUpdate.
    """
    is_block = wrapper.passthrough(BOOL)
    if is_block:
        _block_to_net(wrapper)  # Position


def rewrite_structure_template_data_request(wrapper: PacketWrapper) -> None:
    """StructureTemplateDataRequest (132): Name, Position, StructureSettings, RequestedOperation.

    Args:
        wrapper: Packet wrapper for StructureTemplateDataRequest.
    """
    wrapper.passthrough(STRING)  # Name
    _block_to_net(wrapper)  # Position
    _rewrite_structure_settings(wrapper)


def rewrite_anvil_damage(wrapper: PacketWrapper) -> None:
    """AnvilDamage (141): Damage, Position.

    Args:
        wrapper: Packet wrapper for AnvilDamage.
    """
    wrapper.passthrough(BYTE)  # Damage
    _block_to_net(wrapper)  # Position
