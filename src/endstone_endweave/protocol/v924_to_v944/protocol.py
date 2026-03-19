"""Protocol factory for v924 (r26_u0) server <- v944 (r26_u1) client."""


from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v924_to_v944.handlers.block_pos_packets import (
    rewrite_add_volume_entity,
    rewrite_anvil_damage_sb,
    rewrite_camera_spline,
    rewrite_command_block_update_sb,
    rewrite_container_open_sb,
    rewrite_first_block_to_net_block,
    rewrite_first_net_block_to_block,
    rewrite_player_action_sb,
    rewrite_set_spawn_position,
    rewrite_structure_template_data_request_sb,
    rewrite_update_client_input_locks,
    rewrite_update_sub_chunk_blocks,
)
from endstone_endweave.protocol.v924_to_v944.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
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
    """Create a protocol for v924 server <- v944 client."""
    p = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    # Login
    p.register_serverbound(
        PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings
    )
    p.register_serverbound(PacketId.LOGIN, rewrite_login)

    # Cancel new v944 serverbound packets unknown to v924 (v924 EndId = 340)
    p.cancel_serverbound(PacketId.SERVERBOUND_DATA_DRIVEN_SCREEN_CLOSED)

    # Clientbound rewriters -- BlockPos conversion (NetworkBlockPos -> BlockPos)
    p.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    p.register_clientbound(PacketId.UPDATE_BLOCK, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.TILE_EVENT, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.SET_SPAWN_POSITION, rewrite_set_spawn_position)
    p.register_clientbound(PacketId.BLOCK_ACTOR_DATA, rewrite_first_net_block_to_block)
    p.register_clientbound(
        PacketId.UPDATE_BLOCK_SYNCED, rewrite_first_net_block_to_block
    )
    p.register_clientbound(PacketId.LECTERN_UPDATE, rewrite_first_net_block_to_block)
    p.register_clientbound(PacketId.ADD_VOLUME_ENTITY, rewrite_add_volume_entity)
    p.register_clientbound(
        PacketId.UPDATE_SUB_CHUNK_BLOCKS, rewrite_update_sub_chunk_blocks
    )
    p.register_clientbound(PacketId.OPEN_SIGN, rewrite_first_net_block_to_block)

    # Clientbound rewriters -- other v944 changes
    p.register_clientbound(
        PacketId.PLAYER_CLIENT_INPUT_PERMISSIONS, rewrite_update_client_input_locks
    )
    p.register_clientbound(PacketId.VOXEL_SHAPES, rewrite_voxel_shapes)
    p.register_clientbound(PacketId.CAMERA_SPLINE, rewrite_camera_spline)

    # Serverbound rewriters -- BlockPos conversion (BlockPos -> NetworkBlockPos)
    p.register_serverbound(PacketId.PLAYER_ACTION, rewrite_player_action_sb)
    p.register_serverbound(PacketId.CONTAINER_OPEN, rewrite_container_open_sb)
    p.register_serverbound(
        PacketId.COMMAND_BLOCK_UPDATE, rewrite_command_block_update_sb
    )
    p.register_serverbound(
        PacketId.STRUCTURE_BLOCK_UPDATE, rewrite_first_block_to_net_block
    )
    p.register_serverbound(
        PacketId.STRUCTURE_TEMPLATE_DATA_EXPORT_REQUEST,
        rewrite_structure_template_data_request_sb,
    )
    p.register_serverbound(PacketId.ANVIL_DAMAGE, rewrite_anvil_damage_sb)

    return p
