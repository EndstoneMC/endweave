"""Packet handlers for BlockPos changes (v944 -> v924)."""

from endstone_endweave.codec import (
    BLOCK_POS,
    BOOL,
    BYTE,
    FLOAT_LE,
    INT_LE,
    ITEM_INSTANCE,
    NETWORK_BLOCK_POS,
    STRING,
    UVAR_INT,
    UVAR_INT64,
    VAR_INT,
    VAR_INT64,
    PacketWrapper,
)

_NOTE_BLOCK_EVENT = 0
_TRUMPET_INSERTION_POINT = 16
_TRUMPET_ID_SHIFT = 4


def _block_to_net(wrapper: PacketWrapper) -> None:
    wrapper.write(NETWORK_BLOCK_POS, wrapper.read(BLOCK_POS))


def _net_to_block(wrapper: PacketWrapper) -> None:
    wrapper.write(BLOCK_POS, wrapper.read(NETWORK_BLOCK_POS))


def _passthrough_inventory_action(wrapper: PacketWrapper) -> None:
    source_type = wrapper.passthrough(UVAR_INT)
    if source_type in (0, 99999):
        wrapper.passthrough(VAR_INT)
    elif source_type == 2:
        wrapper.passthrough(UVAR_INT)
    wrapper.passthrough(UVAR_INT)
    wrapper.passthrough(ITEM_INSTANCE)
    wrapper.passthrough(ITEM_INSTANCE)


def _passthrough_structure_settings_up(wrapper: PacketWrapper) -> None:
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    _net_to_block(wrapper)
    _net_to_block(wrapper)
    wrapper.passthrough(VAR_INT64)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(INT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)


def rewrite_first_block_to_net(wrapper: PacketWrapper) -> None:
    """Rewrite first-field BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper positioned at the first field.
    """
    _block_to_net(wrapper)


def rewrite_tile_event(wrapper: PacketWrapper) -> None:
    """BlockEventPacket (26): convert BlockPos -> NetworkBlockPos, remap NoteBlockInstrument.

    Args:
        wrapper: Packet wrapper for TileEvent.
    """
    _block_to_net(wrapper)
    event_type = wrapper.passthrough(VAR_INT)
    event_data = wrapper.read(VAR_INT)
    if event_type == _NOTE_BLOCK_EVENT and event_data >= _TRUMPET_INSERTION_POINT + _TRUMPET_ID_SHIFT:
        event_data -= _TRUMPET_ID_SHIFT
    wrapper.write(VAR_INT, event_data)


def rewrite_set_spawn_position(wrapper: PacketWrapper) -> None:
    """SetSpawnPosition (43): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for SetSpawnPosition.
    """
    wrapper.passthrough(VAR_INT)
    _block_to_net(wrapper)
    wrapper.passthrough(VAR_INT)
    _block_to_net(wrapper)


def rewrite_add_volume_entity(wrapper: PacketWrapper) -> None:
    """AddVolumeEntity (166): convert bounds BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for AddVolumeEntity.
    """
    wrapper.passthrough(UVAR_INT)
    _block_to_net(wrapper)
    _block_to_net(wrapper)


def rewrite_update_sub_chunk_blocks(wrapper: PacketWrapper) -> None:
    """UpdateSubChunkBlocks (172): convert all BlockPos fields.

    Args:
        wrapper: Packet wrapper for UpdateSubChunkBlocks.
    """
    _block_to_net(wrapper)
    blocks_count = wrapper.passthrough(UVAR_INT)
    for _ in range(blocks_count):
        _block_to_net(wrapper)
        wrapper.passthrough(UVAR_INT)
        wrapper.passthrough(UVAR_INT)
        wrapper.passthrough(UVAR_INT64)
        wrapper.passthrough(UVAR_INT)

    extra_count = wrapper.passthrough(UVAR_INT)
    for _ in range(extra_count):
        _block_to_net(wrapper)
        wrapper.passthrough(UVAR_INT)
        wrapper.passthrough(UVAR_INT)
        wrapper.passthrough(UVAR_INT64)
        wrapper.passthrough(UVAR_INT)


def rewrite_play_sound(wrapper: PacketWrapper) -> None:
    """PlaySound (86): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for PlaySound.
    """
    wrapper.passthrough(STRING)
    _block_to_net(wrapper)


def rewrite_map_data(wrapper: PacketWrapper) -> None:
    """ClientboundMapItemData (67): convert tracked block object positions.

    Args:
        wrapper: Packet wrapper for ClientboundMapItemData.
    """
    wrapper.passthrough(VAR_INT64)
    types = wrapper.passthrough(UVAR_INT)
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(NETWORK_BLOCK_POS)

    type_texture_update = 0x02
    type_decoration_update = 0x04
    type_creation = 0x08

    if types & type_creation:
        count = wrapper.passthrough(UVAR_INT)
        for _ in range(count):
            wrapper.passthrough(VAR_INT64)

    if types & (type_creation | type_decoration_update | type_texture_update):
        wrapper.passthrough(BYTE)

    if types & type_decoration_update:
        object_count = wrapper.passthrough(UVAR_INT)
        for _ in range(object_count):
            object_type = wrapper.passthrough(INT_LE)
            if object_type == 0:
                wrapper.passthrough(VAR_INT64)
            elif object_type == 1:
                _block_to_net(wrapper)


def rewrite_update_client_input_locks(wrapper: PacketWrapper) -> None:
    """UpdateClientInputLocks (196): append removed Server Pos.

    Args:
        wrapper: Packet wrapper for UpdateClientInputLocks.
    """
    wrapper.passthrough(UVAR_INT)
    wrapper.write(FLOAT_LE, 0.0)
    wrapper.write(FLOAT_LE, 0.0)
    wrapper.write(FLOAT_LE, 0.0)


def rewrite_inventory_transaction(wrapper: PacketWrapper) -> None:
    """Rewrite InventoryTransaction: convert NetworkBlockPos -> BlockPos in UseItem data.

    Args:
        wrapper: Packet wrapper for InventoryTransaction.
    """
    legacy_request_id = wrapper.passthrough(VAR_INT)
    if legacy_request_id != 0:
        slot_count = wrapper.passthrough(UVAR_INT)
        for _ in range(slot_count):
            wrapper.passthrough(BYTE)
            slots_len = wrapper.passthrough(UVAR_INT)
            for _ in range(slots_len):
                wrapper.passthrough(BYTE)

    transaction_type = wrapper.passthrough(UVAR_INT)
    action_count = wrapper.passthrough(UVAR_INT)
    for _ in range(action_count):
        _passthrough_inventory_action(wrapper)

    if transaction_type != 2:
        return

    wrapper.passthrough(UVAR_INT)
    wrapper.passthrough(UVAR_INT)
    _net_to_block(wrapper)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(VAR_INT)
    wrapper.passthrough(ITEM_INSTANCE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(FLOAT_LE)
    wrapper.passthrough(UVAR_INT)
    wrapper.passthrough(UVAR_INT)
    wrapper.write(BYTE, 0)


def rewrite_player_action(wrapper: PacketWrapper) -> None:
    """PlayerAction (36): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for PlayerAction.
    """
    wrapper.passthrough(UVAR_INT64)
    wrapper.passthrough(VAR_INT)
    _net_to_block(wrapper)
    _net_to_block(wrapper)


def rewrite_container_open(wrapper: PacketWrapper) -> None:
    """ContainerOpen (46): convert BlockPos -> NetworkBlockPos.

    Args:
        wrapper: Packet wrapper for ContainerOpen.
    """
    wrapper.passthrough(BYTE)
    wrapper.passthrough(BYTE)
    _block_to_net(wrapper)


def rewrite_structure_block_update(wrapper: PacketWrapper) -> None:
    """StructureBlockUpdate (90): convert NetworkBlockPos -> BlockPos in StructureSettings.

    Args:
        wrapper: Packet wrapper for StructureBlockUpdate.
    """
    _net_to_block(wrapper)
    wrapper.passthrough(STRING)
    wrapper.passthrough(STRING)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(BOOL)
    wrapper.passthrough(VAR_INT)
    _passthrough_structure_settings_up(wrapper)
    wrapper.passthrough(VAR_INT)


def rewrite_command_block_update(wrapper: PacketWrapper) -> None:
    """CommandBlockUpdate (78): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for CommandBlockUpdate.
    """
    is_block = wrapper.passthrough(BOOL)
    if is_block:
        _net_to_block(wrapper)


def rewrite_structure_template_data_request(wrapper: PacketWrapper) -> None:
    """StructureTemplateDataRequest (132): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for StructureTemplateDataRequest.
    """
    wrapper.passthrough(STRING)
    _net_to_block(wrapper)
    _passthrough_structure_settings_up(wrapper)


def rewrite_anvil_damage(wrapper: PacketWrapper) -> None:
    """AnvilDamage (141): convert NetworkBlockPos -> BlockPos.

    Args:
        wrapper: Packet wrapper for AnvilDamage.
    """
    wrapper.passthrough(BYTE)
    _net_to_block(wrapper)
