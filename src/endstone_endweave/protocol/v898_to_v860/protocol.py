"""Protocol factory for v898 (1.21.130) server <- v860 (1.21.124) client."""

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId
from endstone_endweave.protocol.v898_to_v860.handlers.commands import (
    rewrite_available_commands,
    rewrite_command_output,
    rewrite_command_request,
    rewrite_text_clientbound,
    rewrite_text_serverbound,
)
from endstone_endweave.protocol.v898_to_v860.handlers.gameplay import (
    rewrite_animate_clientbound,
    rewrite_animate_serverbound,
    rewrite_camera_aim_assist_presets,
    rewrite_event,
    rewrite_interact,
    rewrite_mob_effect,
    rewrite_resource_pack_stack,
    rewrite_start_game,
)
from endstone_endweave.protocol.v898_to_v860.handlers.login import (
    rewrite_login,
    rewrite_request_network_settings,
)
from endstone_endweave.protocol.v898_to_v860.handlers.sound_event import (
    rewrite_actor_event,
    rewrite_add_actor,
    rewrite_add_item_actor,
    rewrite_add_player,
    rewrite_level_sound_event,
    rewrite_set_actor_data,
)

SERVER_PROTOCOL = 898
CLIENT_PROTOCOL = 860


def create_protocol() -> Protocol:
    """Create a protocol for v898 server <- v860 client translation."""
    protocol = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    protocol.register_serverbound(PacketId.REQUEST_NETWORK_SETTINGS, rewrite_request_network_settings)
    protocol.register_serverbound(PacketId.LOGIN, rewrite_login)
    protocol.register_serverbound(PacketId.ANIMATE, rewrite_animate_clientbound)
    protocol.register_serverbound(PacketId.INTERACT, rewrite_interact)
    protocol.register_serverbound(PacketId.COMMAND_REQUEST, rewrite_command_request)
    protocol.register_serverbound(PacketId.TEXT, rewrite_text_clientbound)

    protocol.register_clientbound(PacketId.ADD_PLAYER, rewrite_add_player)
    protocol.register_clientbound(PacketId.ADD_ACTOR, rewrite_add_actor)
    protocol.register_clientbound(PacketId.ADD_ITEM_ACTOR, rewrite_add_item_actor)
    protocol.register_clientbound(PacketId.ACTOR_EVENT, rewrite_actor_event)
    protocol.register_clientbound(PacketId.SET_ACTOR_DATA, rewrite_set_actor_data)
    protocol.register_clientbound(PacketId.ANIMATE, rewrite_animate_serverbound)
    protocol.register_clientbound(PacketId.MOB_EFFECT, rewrite_mob_effect)
    protocol.register_clientbound(PacketId.RESOURCE_PACK_STACK, rewrite_resource_pack_stack)
    protocol.register_clientbound(PacketId.TEXT, rewrite_text_serverbound)
    protocol.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    protocol.register_clientbound(PacketId.LEGACY_TELEMETRY_EVENT, rewrite_event)
    protocol.register_clientbound(PacketId.AVAILABLE_COMMANDS, rewrite_available_commands)
    protocol.register_clientbound(PacketId.COMMAND_OUTPUT, rewrite_command_output)
    protocol.register_clientbound(PacketId.LEVEL_SOUND_EVENT, rewrite_level_sound_event)
    protocol.register_clientbound(PacketId.CAMERA_AIM_ASSIST_PRESETS, rewrite_camera_aim_assist_presets)

    protocol.cancel_clientbound(PacketId.CLIENTBOUND_DATA_STORE)

    return protocol
