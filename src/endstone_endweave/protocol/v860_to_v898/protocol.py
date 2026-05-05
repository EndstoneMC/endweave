"""Protocol factory for v860 (1.21.124) server <- v898 (1.21.130) client."""

from ...rewriter import ActorEventRewriter, SoundRewriter
from .. import Protocol
from ..mappings.v860_v898 import MAPPINGS
from ..packet_ids import PacketId
from .handlers.animate import (
    rewrite_animate_clientbound,
    rewrite_animate_serverbound,
)
from .handlers.camera_aim_assist import rewrite_camera_aim_assist_presets
from .handlers.commands import (
    rewrite_available_commands,
    rewrite_command_output,
    rewrite_command_request,
)
from .handlers.event import rewrite_event
from .handlers.interact import rewrite_interact
from .handlers.mob_effect import rewrite_mob_effect
from .handlers.resource_pack_stack import rewrite_resource_pack_stack
from .handlers.start_game import rewrite_start_game
from .handlers.text import (
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)

SERVER_PROTOCOL = 860
CLIENT_PROTOCOL = 898

def create_protocol() -> Protocol:
    """Create a protocol for v860 server <- v898 client translation."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.cancel_serverbound(PacketId.SERVERBOUND_DATA_STORE)
    protocol.register_serverbound(PacketId.ANIMATE, rewrite_animate_serverbound)
    protocol.register_serverbound(PacketId.INTERACT, rewrite_interact)
    protocol.register_serverbound(PacketId.COMMAND_REQUEST, rewrite_command_request)
    protocol.register_serverbound(PacketId.TEXT, rewrite_text_serverbound)

    sound = SoundRewriter(
        sound_remap=MAPPINGS.sound.shift_up,
        actor_data_int_remappers={MAPPINGS.actor_data_sound_key: MAPPINGS.sound.shift_up},
    )
    sound.register(protocol)

    assert MAPPINGS.actor_event is not None
    ActorEventRewriter(MAPPINGS.actor_event, upgrade=True).register(protocol)

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
