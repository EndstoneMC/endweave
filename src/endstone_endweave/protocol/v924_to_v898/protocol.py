"""Protocol factory for v924 (1.26.0) server <- v898 (1.21.130) client."""

from ...rewriter import SoundRewriter
from .. import Protocol
from ..mappings.v898_v924 import MAPPINGS
from ..packet_ids import PacketId
from .handlers.biome_definition_list import (
    rewrite_biome_definition_list,
)
from .handlers.book_edit import (
    rewrite_book_edit,
)
from .handlers.camera import (
    rewrite_camera_instruction,
)
from .handlers.camera_aim_assist import (
    rewrite_camera_aim_assist_presets,
)
from .handlers.data_store import (
    rewrite_clientbound_data_store,
    rewrite_serverbound_data_store,
)
from .handlers.debug_drawer import (
    rewrite_debug_drawer,
)
from .handlers.diagnostics import (
    rewrite_diagnostics,
)
from .handlers.graphics_parameter_override import (
    rewrite_graphics_parameter_override,
)
from .handlers.start_game import (
    rewrite_start_game,
)
from .handlers.text import (
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
    protocol.register_serverbound(PacketId.BOOK_EDIT, rewrite_book_edit)
    protocol.register_serverbound(PacketId.SERVERBOUND_DIAGNOSTICS, rewrite_diagnostics)

    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_clientbound)
    protocol.register_clientbound(PacketId.CLIENTBOUND_DATA_STORE, rewrite_clientbound_data_store)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)
    protocol.register_clientbound(PacketId.BIOME_DEFINITION_LIST, rewrite_biome_definition_list)

    sound = SoundRewriter(
        sound_remap=MAPPINGS.sound.cap,
        actor_data_int_remappers={MAPPINGS.actor_data_sound_key: MAPPINGS.sound.cap},
        dropped_actor_data_keys=MAPPINGS.dropped_actor_data_keys,
    )
    sound.register(protocol)

    protocol.register_clientbound(PacketId.GRAPHICS_PARAMETER_OVERRIDE, rewrite_graphics_parameter_override)
    protocol.register_clientbound(PacketId.CAMERA_INSTRUCTION, rewrite_camera_instruction)
    protocol.register_clientbound(PacketId.SERVER_SCRIPT_DEBUG_DRAWER, rewrite_debug_drawer)

    # Unknown to v898
    protocol.cancel_clientbound(
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_SHOW_SCREEN,
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_CLOSE_ALL_SCREENS,
        PacketId.CLIENTBOUND_DATA_DRIVEN_UI_RELOAD,
        PacketId.CLIENTBOUND_TEXTURE_SHIFT,
        PacketId.VOXEL_SHAPES,
        PacketId.CAMERA_SPLINE,
        PacketId.CAMERA_AIM_ASSIST_ACTOR_PRIORITY,
    )

    return protocol
