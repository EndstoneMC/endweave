"""Packet handlers for NetworkBlockPos -> BlockPos changes (v924 -> v944).

Clientbound handlers read NetworkBlockPos (from v924 server) and write BlockPos (for v944 client).
Serverbound handlers read BlockPos (from v944 client) and write NetworkBlockPos (for v924 server).
"""

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    FLOAT_LE,
    STRING,
    NETWORK_BLOCK_POS,
    UINT_LE,
    UVAR_INT,
    UVAR_LONG,
    VAR_INT,
    VAR_LONG,
    PacketWrapper,
)


# ---------------------------------------------------------------------------
# Clientbound (server -> client): NetworkBlockPos -> BlockPos
# ---------------------------------------------------------------------------


def rewrite_first_net_block_to_block(wrapper: PacketWrapper) -> None:
    """Rewrite first-field NetworkBlockPos -> BlockPos.

    Used by: UpdateBlock (21), BlockEvent (26), BlockActorData (56),
    UpdateBlockSynced (110), LecternUpdate (125), OpenSign (303).
    """
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))


def rewrite_set_spawn_position(wrapper: PacketWrapper) -> None:
    """SetSpawnPosition (43): spawnType, Position, dimension, SpawnPosition."""
    wrapper.passthrough(VAR_INT)  # spawnType
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # Position
    wrapper.passthrough(VAR_INT)  # dimension
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # SpawnPosition


def rewrite_add_volume_entity(wrapper: PacketWrapper) -> None:
    """AddVolumeEntity (166): RuntimeID, Bounds[0], Bounds[1], then rest."""
    wrapper.passthrough(UVAR_INT)  # RuntimeID
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # MinBound
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # MaxBound


def rewrite_update_sub_chunk_blocks(wrapper: PacketWrapper) -> None:
    """UpdateSubChunkBlocks (172): Position, then Blocks/Extra slices.

    Each slice is uvarint count + BlockChangeEntry[].
    BlockChangeEntry: BlockPos + uvarint (blockRuntimeID) + uvarint (flags) + uvarint64 (syncedUpdateEntityUniqueID).
    """
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # SubChunk position

    # Blocks slice
    blocks_count = wrapper.passthrough(UVAR_INT)
    for _ in range(blocks_count):
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # BlockPos
        wrapper.passthrough(UVAR_INT)  # blockRuntimeID
        wrapper.passthrough(UVAR_INT)  # flags
        wrapper.passthrough(UVAR_LONG)  # syncedUpdateEntityUniqueID

    # Extra slice
    extra_count = wrapper.passthrough(UVAR_INT)
    for _ in range(extra_count):
        wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # BlockPos
        wrapper.passthrough(UVAR_INT)  # blockRuntimeID
        wrapper.passthrough(UVAR_INT)  # flags
        wrapper.passthrough(UVAR_LONG)  # syncedUpdateEntityUniqueID


def rewrite_play_sound(wrapper: PacketWrapper) -> None:
    """PlaySound (86): Name, Position, Volume, Pitch."""
    wrapper.passthrough(STRING)  # Name
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))  # Position


def rewrite_update_client_input_locks(wrapper: PacketWrapper) -> None:
    """UpdateClientInputLocks (196): v944 removed trailing Vec3 (12 bytes).

    v924: varuint32 Locks + Vec3 (3x float32)
    v944: varuint32 Locks only
    """
    wrapper.passthrough(UVAR_INT)  # Locks
    wrapper.read(FLOAT_LE)  # discard Position.X
    wrapper.read(FLOAT_LE)  # discard Position.Y
    wrapper.read(FLOAT_LE)  # discard Position.Z


def rewrite_camera_spline(wrapper: PacketWrapper) -> None:
    """CameraSpline (338): v944 added optional SplineIdentifier + optional LoadFromJson.

    Append false + false (both optionals absent).
    """
    wrapper.passthrough_all()
    wrapper.write(BOOL, False)  # has SplineIdentifier
    wrapper.write(BOOL, False)  # has LoadFromJson


# ---------------------------------------------------------------------------
# Serverbound (client -> server): BlockPos -> NetworkBlockPos
# ---------------------------------------------------------------------------


def rewrite_player_action(wrapper: PacketWrapper) -> None:
    """PlayerAction (36): entityRuntimeID, actionType, BlockPosition, ResultPosition, face."""
    wrapper.passthrough(UVAR_LONG)  # entityRuntimeID
    wrapper.passthrough(VAR_INT)  # actionType
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # BlockPosition
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # ResultPosition


def rewrite_container_open(wrapper: PacketWrapper) -> None:
    """ContainerOpen (46): windowID, type, ContainerPosition, entityUniqueID."""
    wrapper.passthrough(BYTE)  # windowID
    wrapper.passthrough(BYTE)  # type
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # ContainerPosition


def _rewrite_structure_settings(wrapper: PacketWrapper) -> None:
    """Passthrough StructureSettings, converting BlockPos -> NetworkBlockPos.

    Layout: string PaletteName, bool IgnoreEntities, bool IgnoreBlocks,
    bool AllowNonTickingChunks, BlockPos Size, BlockPos Offset,
    varint64 LastEditingPlayerUniqueID, byte Rotation, byte Mirror,
    byte AnimationMode, float AnimationSeconds, float IntegrityValue,
    uint32 IntegritySeed, Vec3 RotationPivot.
    """
    wrapper.passthrough(STRING)  # PaletteName
    wrapper.passthrough(BOOL)  # IgnoreEntities
    wrapper.passthrough(BOOL)  # IgnoreBlocks
    wrapper.passthrough(BOOL)  # AllowNonTickingChunks
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # Size
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # Offset
    wrapper.passthrough(VAR_LONG)  # LastEditingPlayerUniqueID
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
    """
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # Block Position
    # StructureEditorData
    wrapper.passthrough(STRING)  # Name
    wrapper.passthrough(STRING)  # DataField
    wrapper.passthrough(BOOL)  # IncludePlayers
    wrapper.passthrough(BOOL)  # ShowBoundingBox
    wrapper.passthrough(VAR_INT)  # StructureBlockType
    _rewrite_structure_settings(wrapper)
    wrapper.passthrough(VAR_INT)  # RedstoneSaveMode


def rewrite_command_block_update(wrapper: PacketWrapper) -> None:
    """CommandBlockUpdate (78): IsBlock (bool), then if true: Position first."""
    is_block = wrapper.passthrough(BOOL)
    if is_block:
        wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # Position


def rewrite_structure_template_data_request(wrapper: PacketWrapper) -> None:
    """StructureTemplateDataRequest (132): Name, Position, StructureSettings, RequestedOperation."""
    wrapper.passthrough(STRING)  # Name
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # Position
    _rewrite_structure_settings(wrapper)


def rewrite_anvil_damage(wrapper: PacketWrapper) -> None:
    """AnvilDamage (141): Damage, Position."""
    wrapper.passthrough(BYTE)  # Damage
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))  # Position
