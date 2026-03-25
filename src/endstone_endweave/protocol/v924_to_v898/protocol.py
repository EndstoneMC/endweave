"""Protocol factory for v924 (1.26.0) server <- v898 (1.21.130) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v924_to_v898.handlers.biome_definition_list import (
    rewrite_biome_definition_list,
)
from endstone_endweave.protocol.v924_to_v898.handlers.data_store import (
    rewrite_clientbound_data_store,
    rewrite_serverbound_data_store,
)
from endstone_endweave.protocol.v924_to_v898.handlers.gameplay import (
    rewrite_camera_aim_assist_presets,
    rewrite_graphics_parameter_override,
    rewrite_start_game,
)
from endstone_endweave.protocol.v924_to_v898.handlers.sound_event import (
    rewrite_add_actor,
    rewrite_add_item_actor,
    rewrite_add_player,
    rewrite_level_sound_event,
    rewrite_set_actor_data,
)
from endstone_endweave.protocol.v924_to_v898.handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

SERVER_PROTOCOL = 924
CLIENT_PROTOCOL = 898


def create_protocol() -> Protocol:
    """Create a protocol for v924 server <- v898 client."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.TEXT, rewrite_text_serverbound)
    protocol.register_serverbound(PacketId.SERVERBOUND_DATA_STORE, rewrite_serverbound_data_store)

    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_clientbound)
    protocol.register_clientbound(PacketId.CLIENTBOUND_DATA_STORE, rewrite_clientbound_data_store)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)
    protocol.register_clientbound(PacketId.BIOME_DEFINITION_LIST, rewrite_biome_definition_list)
    protocol.register_clientbound(PacketId.LEVEL_SOUND_EVENT, rewrite_level_sound_event)
    protocol.register_clientbound(PacketId.ADD_PLAYER, rewrite_add_player)
    protocol.register_clientbound(PacketId.ADD_ACTOR, rewrite_add_actor)
    protocol.register_clientbound(PacketId.ADD_ITEM_ACTOR, rewrite_add_item_actor)
    protocol.register_clientbound(PacketId.SET_ACTOR_DATA, rewrite_set_actor_data)
    protocol.register_clientbound(PacketId.GRAPHICS_PARAMETER_OVERRIDE, rewrite_graphics_parameter_override)

    protocol.cancel_clientbound(
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_SHOW_SCREEN,
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_CLOSE_ALL_SCREENS,
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_RELOAD,
        PacketId.CLIENTBOUND_TEXTURE_SHIFT,
        PacketId.VOXEL_SHAPES,
        PacketId.CAMERA_SPLINE,
        PacketId.CAMERA_AIM_ASSIST_ACTOR_PRIORITY,
        PacketId.CAMERA_INSTRUCTION,
        PacketId.SERVER_SCRIPT_DEBUG_DRAWER,
    )

    return protocol
