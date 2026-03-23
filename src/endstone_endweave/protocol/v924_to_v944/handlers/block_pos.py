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
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)
from endstone_endweave.protocol.rewriter import (
    block_to_net as _block_to_net,
)
from endstone_endweave.protocol.rewriter import (
    net_to_block as _net_to_block,
)
from endstone_endweave.protocol.rewriter import (
    passthrough_inventory_action as _passthrough_inventory_action,
)
from endstone_endweave.protocol.rewriter import (
    passthrough_structure_settings as _passthrough_structure_settings,
)

# NoteBlockInstrument remapping constants (TileEvent)
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
    _net_to_block(wrapper)


def rewrite_tile_event(wrapper: PacketWrapper) -> None:
    """BlockEventPacket (26): convert NetworkBlockPos -> BlockPos, remap NoteBlockInstrument.

    v944 inserted Trumpet variants at IDs 16-19, displacing Zombie..Piglin by +4.

    Args:
        wrapper: Packet wrapper for TileEvent.
    """
    _net_to_block(wrapper)  # Block Position
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
    _net_to_block(wrapper)  # Block Position
    wrapper.passthrough(VAR_INT)  # Dimension type
    _net_to_block(wrapper)  # Spawn Block Pos


def rewrite_add_volume_entity(wrapper: PacketWrapper) -> None:
    """AddVolumeEntity (166): convert NetworkBlockPos -> BlockPos in bounds.

    Args:
        wrapper: Packet wrapper for AddVolumeEntity.
    """
    wrapper.passthrough(UVAR_INT)  # Entity Network Id
    _net_to_block(wrapper)  # Min Bounds
    _net_to_block(wrapper)  # Max Bounds


def rewrite_update_sub_chunk_blocks(wrapper: PacketWrapper) -> None:
    """UpdateSubChunkBlocks (172): convert NetworkBlockPos -> BlockPos in all entries.

    Args:
        wrapper: Packet wrapper for UpdateSubChunkBlocks.
    """
    _net_to_block(wrapper)  # Sub Chunk Block Position

    # Blocks Changed - Standards
    blocks_count = wrapper.passthrough(UVAR_INT)
    for _ in range(blocks_count):
        _net_to_block(wrapper)  # Pos
        wrapper.passthrough(UVAR_INT)  # Runtime Id
        wrapper.passthrough(UVAR_INT)  # Update Flags
        wrapper.passthrough(UVAR_INT64)  # Sync Message - Entity Unique ID
        wrapper.passthrough(UVAR_INT)  # Sync Message - Message

    # Blocks Changed - Extras
    extra_count = wrapper.passthrough(UVAR_INT)
    for _ in range(extra_count):
        _net_to_block(wrapper)  # Pos
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
    _net_to_block(wrapper)  # Position


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

    TYPE_TEXTURE_UPDATE = 0x02
    TYPE_DECORATION_UPDATE = 0x04
    TYPE_CREATION = 0x08

    if types & TYPE_CREATION:
        # Map ID List
        count = wrapper.passthrough(UVAR_INT)
        for _ in range(count):
            wrapper.passthrough(VAR_INT64)  # Map ID entry

    if types & (TYPE_CREATION | TYPE_DECORATION_UPDATE | TYPE_TEXTURE_UPDATE):
        wrapper.passthrough(BYTE)  # Scale

    if types & TYPE_DECORATION_UPDATE:
        # Actor IDs
        obj_count = wrapper.passthrough(UVAR_INT)
        for _ in range(obj_count):
            obj_type = wrapper.passthrough(INT_LE)  # Type
            if obj_type == 0:  # Entity
                wrapper.passthrough(VAR_INT64)  # MapItemTrackedActor::UniqueId
            elif obj_type == 1:  # Block
                _net_to_block(wrapper)  # Block Position


def rewrite_update_client_input_locks(wrapper: PacketWrapper) -> None:
    """UpdateClientInputLocks (196): strip trailing Server Pos removed in v944.

    Args:
        wrapper: Packet wrapper for UpdateClientInputLocks.
    """
    wrapper.passthrough(UVAR_INT)  # Input Lock ComponentData
    wrapper.read(FLOAT_LE)  # discard Server Pos.X
    wrapper.read(FLOAT_LE)  # discard Server Pos.Y
    wrapper.read(FLOAT_LE)  # discard Server Pos.Z


def rewrite_camera_spline(wrapper: PacketWrapper) -> None:
    """CameraSpline (338): append SplineIdentifier + LoadFromJson optionals added in v944.

    Args:
        wrapper: Packet wrapper for CameraSpline.
    """
    wrapper.passthrough_all()
    wrapper.write(BOOL, False)  # has SplineIdentifier
    wrapper.write(BOOL, False)  # has LoadFromJson


# ---------------------------------------------------------------------------
# Serverbound (client -> server): BlockPos -> NetworkBlockPos
# ---------------------------------------------------------------------------


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
    """PlayerAction (36): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for PlayerAction.
    """
    wrapper.passthrough(UVAR_INT64)  # Player Runtime ID
    wrapper.passthrough(VAR_INT)  # Action
    _block_to_net(wrapper)  # Block Position
    _block_to_net(wrapper)  # Result Pos


def rewrite_container_open(wrapper: PacketWrapper) -> None:
    """ContainerOpen (46): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for ContainerOpen.
    """
    wrapper.passthrough(BYTE)  # Container Id
    wrapper.passthrough(BYTE)  # Container Type
    _net_to_block(wrapper)  # Position


def rewrite_structure_block_update(wrapper: PacketWrapper) -> None:
    """StructureBlockUpdate (90): convert BlockPos -> NetworkBlockPos in StructureSettings.

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
    _passthrough_structure_settings(wrapper)
    wrapper.passthrough(VAR_INT)  # RedstoneSaveMode


def rewrite_command_block_update(wrapper: PacketWrapper) -> None:
    """CommandBlockUpdate (78): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for CommandBlockUpdate.
    """
    is_block = wrapper.passthrough(BOOL)  # Is Block?
    if is_block:
        _block_to_net(wrapper)  # Block Position


def rewrite_structure_template_data_request(wrapper: PacketWrapper) -> None:
    """StructureTemplateDataRequest (132): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for StructureTemplateDataRequest.
    """
    wrapper.passthrough(STRING)  # Structure Name
    _block_to_net(wrapper)  # Structure Position
    _passthrough_structure_settings(wrapper)


def rewrite_anvil_damage(wrapper: PacketWrapper) -> None:
    """AnvilDamage (141): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for AnvilDamage.
    """
    wrapper.passthrough(BYTE)  # Damage Amount
    _block_to_net(wrapper)  # Block Position
