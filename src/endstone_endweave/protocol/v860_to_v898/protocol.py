"""Protocol factory for v860 (1.21.124) server <- v898 (1.21.130) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.mappings.v860_v898 import MAPPINGS
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.sound_rewriter import SoundRewriter
from endstone_endweave.protocol.v860_to_v898.handlers.actor_event import rewrite_actor_event
from endstone_endweave.protocol.v860_to_v898.handlers.animate import (
    rewrite_animate_clientbound,
    rewrite_animate_serverbound,
)
from endstone_endweave.protocol.v860_to_v898.handlers.camera_aim_assist import rewrite_camera_aim_assist_presets
from endstone_endweave.protocol.v860_to_v898.handlers.commands import (
    rewrite_available_commands,
    rewrite_command_output,
    rewrite_command_request,
)
from endstone_endweave.protocol.v860_to_v898.handlers.event import rewrite_event
from endstone_endweave.protocol.v860_to_v898.handlers.interact import rewrite_interact
from endstone_endweave.protocol.v860_to_v898.handlers.mob_effect import rewrite_mob_effect
from endstone_endweave.protocol.v860_to_v898.handlers.resource_pack_stack import rewrite_resource_pack_stack
from endstone_endweave.protocol.v860_to_v898.handlers.start_game import rewrite_start_game
from endstone_endweave.protocol.v860_to_v898.handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

SERVER_PROTOCOL = 860
CLIENT_PROTOCOL = 898

def create_protocol() -> Protocol:
    """Create a protocol for v860 server <- v898 client translation."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    # Serverbound rewriters
    protocol.cancel_serverbound(PacketId.SERVERBOUND_DATA_STORE)
    protocol.register_serverbound(PacketId.ANIMATE, rewrite_animate_serverbound)
    protocol.register_serverbound(PacketId.INTERACT, rewrite_interact)
    protocol.register_serverbound(PacketId.COMMAND_REQUEST, rewrite_command_request)
    protocol.register_serverbound(PacketId.TEXT, rewrite_text_serverbound)

    # Clientbound rewriters -- LevelSoundEvent remapping
    sound = SoundRewriter(
        sound_remap=MAPPINGS.sound.shift_up,
        actor_data_int_remappers={MAPPINGS.actor_data_sound_key: MAPPINGS.sound.shift_up},
    )
    sound.register(protocol)

    # Clientbound rewriters -- v860/v898 format differences
    protocol.register_clientbound(PacketId.ACTOR_EVENT, rewrite_actor_event)
    protocol.register_clientbound(PacketId.ANIMATE, rewrite_animate_clientbound)
    protocol.register_clientbound(PacketId.MOB_EFFECT, rewrite_mob_effect)
    protocol.register_clientbound(PacketId.RESOURCE_PACK_STACK, rewrite_resource_pack_stack)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_clientbound)
    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.LEGACY_TELEMETRY_EVENT, rewrite_event)
    protocol.register_clientbound(PacketId.AVAILABLE_COMMANDS, rewrite_available_commands)
    protocol.register_clientbound(PacketId.COMMAND_OUTPUT, rewrite_command_output)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)

    return protocol
