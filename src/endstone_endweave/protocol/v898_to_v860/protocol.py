"""Protocol factory for v898 (1.21.130) server <- v860 (1.21.124) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.sound_rewriter import SoundRewriter
from endstone_endweave.protocol.v898_to_v860.handlers.commands import (
    rewrite_available_commands,
    rewrite_command_output,
    rewrite_command_request,
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)
from endstone_endweave.protocol.v898_to_v860.handlers.gameplay import (
    rewrite_actor_event,
    rewrite_animate_clientbound,
    rewrite_animate_serverbound,
    rewrite_camera_aim_assist_presets,
    rewrite_event,
    rewrite_interact,
    rewrite_mob_effect,
    rewrite_resource_pack_stack,
    rewrite_start_game,
)

SERVER_PROTOCOL = 898
CLIENT_PROTOCOL = 860

# v898 inserted 12 sounds at 566; v860 fallback at 578
_HEARTBEAT = 127
_SOUND_INSERT_AT = 566
_OLD_SOUND_FALLBACK = 578
_SOUND_SHIFT = 12


def _remap_sound(v: int) -> int:
    """Remap LevelSoundEvent from v898 -> v860 (collapse inserted sounds)."""
    if v >= _OLD_SOUND_FALLBACK:
        return v - _SOUND_SHIFT
    if v >= _SOUND_INSERT_AT:
        return _OLD_SOUND_FALLBACK
    return v


def create_protocol() -> Protocol:
    """Create a protocol for v898 server <- v860 client translation."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.ANIMATE, rewrite_animate_clientbound)
    protocol.register_serverbound(PacketId.INTERACT, rewrite_interact)
    protocol.register_serverbound(PacketId.COMMAND_REQUEST, rewrite_command_request)
    protocol.register_serverbound(PacketId.TEXT, rewrite_text_clientbound)

    sound = SoundRewriter(
        sound_remap=_remap_sound,
        actor_data_int_remappers={_HEARTBEAT: _remap_sound},
    )
    sound.register(protocol)
    protocol.register_clientbound(PacketId.ACTOR_EVENT, rewrite_actor_event)
    protocol.register_clientbound(PacketId.ANIMATE, rewrite_animate_serverbound)
    protocol.register_clientbound(PacketId.MOB_EFFECT, rewrite_mob_effect)
    protocol.register_clientbound(PacketId.RESOURCE_PACK_STACK, rewrite_resource_pack_stack)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_serverbound)
    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.LEGACY_TELEMETRY_EVENT, rewrite_event)
    protocol.register_clientbound(PacketId.AVAILABLE_COMMANDS, rewrite_available_commands)
    protocol.register_clientbound(PacketId.COMMAND_OUTPUT, rewrite_command_output)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)

    protocol.cancel_clientbound(PacketId.CLIENTBOUND_DATA_STORE)

    return protocol
