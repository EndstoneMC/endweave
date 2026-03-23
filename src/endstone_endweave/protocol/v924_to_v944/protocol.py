"""Protocol factory for v924 (r26_u0) server <- v944 (r26_u1) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v924_to_v944.handlers.block_pos import (
    rewrite_add_volume_entity,
    rewrite_anvil_damage,
    rewrite_camera_spline,
    rewrite_command_block_update,
    rewrite_container_open,
    rewrite_first_net_block_to_block,
    rewrite_inventory_transaction,
    rewrite_map_data,
    rewrite_play_sound,
    rewrite_player_action,
    rewrite_set_spawn_position,
    rewrite_structure_block_update,
    rewrite_structure_template_data_request,
    rewrite_tile_event,
    rewrite_update_client_input_locks,
    rewrite_update_sub_chunk_blocks,
)
from endstone_endweave.protocol.v924_to_v944.handlers.data_driven_ui import (
    rewrite_close_all_screens,
    rewrite_show_screen,
)
from endstone_endweave.protocol.v924_to_v944.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)
from endstone_endweave.protocol.v924_to_v944.handlers.sound_event import (
    rewrite_add_actor,
    rewrite_add_item_actor,
    rewrite_add_player,
    rewrite_level_sound_event,
    rewrite_set_actor_data,
)
from endstone_endweave.protocol.v924_to_v944.handlers.start_game import (
    rewrite_start_game,
)
from endstone_endweave.protocol.v924_to_v944.handlers.voxel_shapes import (
    rewrite_voxel_shapes,
)

SERVER_PROTOCOL = 924
CLIENT_PROTOCOL = 944


def create_protocol() -> Protocol:
    """Create a protocol for v924 server <- v944 client.

    Returns:
        A Protocol instance with all v924-to-v944 handlers registered.
    """
    p = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    # Login
    p.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    p.register_serverbound(PacketId.LOGIN, rewrite_login)

    # Cancel new v944 serverbound packets unknown to v924 (v924 EndId = 340)
    p.cancel_serverbound(
        PacketId.RESOURCE_PACKS_READY_FOR_VALIDATION,  # 340
        PacketId.PARTY_CHANGED,  # 342
        PacketId.SERVERBOUND_DATA_DRIVEN_SCREEN_CLOSED,  # 343
    )

    # Clientbound rewriters -- BlockPos conversion (NetworkBlockPos -> BlockPos)
    p.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    p.register_clientbound(PacketId.UPDATE_BLOCK, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.TILE_EVENT, rewrite_tile_event)
    p.register_clientbound(PacketId.SET_SPAWN_POSITION, rewrite_set_spawn_position)
    p.register_clientbound(PacketId.BLOCK_ACTOR_DATA, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.UPDATE_BLOCK_SYNCED, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.LECTERN_UPDATE, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.ADD_VOLUME_ENTITY, rewrite_add_volume_entity)
    p.register_clientbound(PacketId.UPDATE_SUB_CHUNK_BLOCKS, rewrite_update_sub_chunk_blocks)
    p.register_clientbound(PacketId.OPEN_SIGN, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.PLAY_SOUND, rewrite_play_sound)
    p.register_clientbound(PacketId.MAP_DATA, rewrite_map_data)

    # Clientbound rewriters -- other v944 changes
    p.register_clientbound(PacketId.PLAYER_CLIENT_INPUT_PERMISSIONS, rewrite_update_client_input_locks)
    p.register_clientbound(PacketId.VOXEL_SHAPES, rewrite_voxel_shapes)
    p.register_clientbound(
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_SHOW_SCREEN,
        rewrite_show_screen,
    )
    p.register_clientbound(
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_CLOSE_ALL_SCREENS,
        rewrite_close_all_screens,
    )
    p.register_clientbound(PacketId.CAMERA_SPLINE, rewrite_camera_spline)
    p.register_clientbound(PacketId.CONTAINER_OPEN, rewrite_container_open)

    # Clientbound rewriters -- LevelSoundEvent remapping
    p.register_clientbound(PacketId.LEVEL_SOUND_EVENT, rewrite_level_sound_event)
    p.register_clientbound(PacketId.ADD_PLAYER, rewrite_add_player)
    p.register_clientbound(PacketId.ADD_ACTOR, rewrite_add_actor)
    p.register_clientbound(PacketId.ADD_ITEM_ACTOR, rewrite_add_item_actor)
    p.register_clientbound(PacketId.SET_ACTOR_DATA, rewrite_set_actor_data)

    # Serverbound rewriters -- BlockPos conversion (BlockPos -> NetworkBlockPos)
    p.register_serverbound(PacketId.INVENTORY_TRANSACTION, rewrite_inventory_transaction)
    p.register_serverbound(PacketId.PLAYER_ACTION, rewrite_player_action)
    p.register_serverbound(PacketId.COMMAND_BLOCK_UPDATE, rewrite_command_block_update)
    p.register_serverbound(PacketId.STRUCTURE_BLOCK_UPDATE, rewrite_structure_block_update)
    p.register_serverbound(
        PacketId.STRUCTURE_TEMPLATE_DATA_EXPORT_REQUEST,
        rewrite_structure_template_data_request,
    )
    p.register_serverbound(PacketId.ANVIL_DAMAGE, rewrite_anvil_damage)

    return p
