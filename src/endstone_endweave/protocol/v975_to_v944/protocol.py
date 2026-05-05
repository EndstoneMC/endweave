"""Protocol factory for v975 (r26_u2) server <- v944 (r26_u1) client."""

from ...rewriter import SoundRewriter
from .. import Protocol
from ..mappings.v944_v975 import MAPPINGS
from ..packet_ids import PacketId
from .handlers.actor_event import rewrite_actor_event
from .handlers.diagnostics import rewrite_diagnostics
from .handlers.level_sound_event import rewrite_level_sound_event
from .handlers.mob_equipment import rewrite_mob_equipment_clientbound, rewrite_mob_equipment_serverbound
from .handlers.play_sound import rewrite_play_sound
from .handlers.update_client_options import rewrite_update_client_options

SERVER_PROTOCOL = 975
CLIENT_PROTOCOL = 944


def create_protocol() -> Protocol:
    """Create a protocol for v975 server <- v944 client."""
    p = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    sound = SoundRewriter(
        sound_remap=MAPPINGS.sound.shift_down,
        actor_data_int_remappers={MAPPINGS.actor_data_sound_key: MAPPINGS.sound.shift_down},
    )
    sound.register(p)

    # Override the SoundRewriter's LEVEL_SOUND_EVENT to also strip Fire At Position
    p.register_clientbound(PacketId.LEVEL_SOUND_EVENT, rewrite_level_sound_event)

    p.register_clientbound(PacketId.ACTOR_EVENT, rewrite_actor_event)
    p.register_clientbound(PacketId.PLAY_SOUND, rewrite_play_sound)

    p.register_clientbound(PacketId.PLAYER_EQUIPMENT, rewrite_mob_equipment_clientbound)
    p.register_serverbound(PacketId.PLAYER_EQUIPMENT, rewrite_mob_equipment_serverbound)

    # Unknown to v944 or whose payload was restructured
    p.cancel_clientbound(
        PacketId.SERVER_STORE_INFO,  # 346
        PacketId.SERVER_PRESENCE_INFO,  # 347
        PacketId.LOCATOR_BAR,  # 341 -- TexturePath/IconSize replaced TextureId
        PacketId.SERVER_SCRIPT_DEBUG_DRAWER,  # 328 -- PrimitiveShapeDataPayload restructured
        PacketId.CLIENTBOUND_ATTRIBUTE_LAYER_SYNC,  # 345 -- Weight switch removed
        PacketId.PLAYER_ENCHANT_OPTIONS,  # 146 -- ItemEnchantOption.Cost width changed
    )

    p.register_serverbound(PacketId.UPDATE_CLIENT_OPTIONS, rewrite_update_client_options)
    p.register_serverbound(PacketId.SERVERBOUND_DIAGNOSTICS, rewrite_diagnostics)

    # No safe upgrade
    p.cancel_serverbound(
        PacketId.PARTY_CHANGED,  # 342 -- string -> PlayerPartyInfo struct
    )

    return p
