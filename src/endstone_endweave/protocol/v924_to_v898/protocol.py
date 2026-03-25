"""Protocol factory for v924 (1.26.0) server <- v898 (1.21.130) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.sound_rewriter import SoundRewriter
from endstone_endweave.protocol.v924_to_v898.handlers.biome_definition_list import (
    rewrite_biome_definition_list,
)
from endstone_endweave.protocol.v924_to_v898.handlers.book_edit import (
    rewrite_book_edit,
)
from endstone_endweave.protocol.v924_to_v898.handlers.camera import (
    rewrite_camera_instruction,
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
from endstone_endweave.protocol.v924_to_v898.handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

SERVER_PROTOCOL = 924
CLIENT_PROTOCOL = 898

# v898 Undefined = 578; all v924 sounds >= 578 collapse to Undefined
_HEARTBEAT = 127
_OLD_UNDEFINED = 578
_DROPPED_KEYS = {136, 137, 138}


def _remap_sound(v: int) -> int:
    """Remap LevelSoundEvent from v924 -> v898 (cap at Undefined)."""
    if v >= _OLD_UNDEFINED:
        return _OLD_UNDEFINED
    return v


def create_protocol() -> Protocol:
    """Create a protocol for v924 server <- v898 client."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.TEXT, rewrite_text_serverbound)
    protocol.register_serverbound(PacketId.SERVERBOUND_DATA_STORE, rewrite_serverbound_data_store)
    protocol.register_serverbound(PacketId.BOOK_EDIT, rewrite_book_edit)

    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_clientbound)
    protocol.register_clientbound(PacketId.CLIENTBOUND_DATA_STORE, rewrite_clientbound_data_store)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)
    protocol.register_clientbound(PacketId.BIOME_DEFINITION_LIST, rewrite_biome_definition_list)
    sound = SoundRewriter(
        sound_remap=_remap_sound,
        actor_data_int_remappers={_HEARTBEAT: _remap_sound},
        dropped_actor_data_keys=_DROPPED_KEYS,
    )
    sound.register(protocol)
    protocol.register_clientbound(PacketId.GRAPHICS_PARAMETER_OVERRIDE, rewrite_graphics_parameter_override)
    protocol.register_clientbound(PacketId.CAMERA_INSTRUCTION, rewrite_camera_instruction)

    protocol.cancel_clientbound(
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_SHOW_SCREEN,
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_CLOSE_ALL_SCREENS,
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_RELOAD,
        PacketId.CLIENTBOUND_TEXTURE_SHIFT,
        PacketId.VOXEL_SHAPES,
        PacketId.CAMERA_SPLINE,
        PacketId.CAMERA_AIM_ASSIST_ACTOR_PRIORITY,
        PacketId.SERVER_SCRIPT_DEBUG_DRAWER,
    )

    return protocol
