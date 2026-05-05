"""Protocol factory for v944 (r26_u1) server <- v975 (r26_u2) client."""

from ...rewriter import SoundRewriter
from .. import Protocol
from ..mappings.v944_v975 import MAPPINGS
from ..packet_ids import PacketId
from .handlers.actor_event import rewrite_actor_event
from .handlers.diagnostics import rewrite_diagnostics
from .handlers.item_stack import rewrite_inventory_slot
from .handlers.level_sound_event import rewrite_level_sound_event
from .handlers.mob_equipment import rewrite_mob_equipment_clientbound, rewrite_mob_equipment_serverbound
from .handlers.play_sound import rewrite_play_sound
from .handlers.start_game import rewrite_start_game
from .handlers.update_client_options import rewrite_update_client_options

SERVER_PROTOCOL = 944
CLIENT_PROTOCOL = 975


def create_protocol() -> Protocol:
    """Create a protocol for v944 server <- v975 client.

    Returns:
        A Protocol instance with all v944-to-v975 handlers registered.
    """
    p = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL)

    sound = SoundRewriter(
        sound_remap=MAPPINGS.sound.shift_up,
        actor_data_int_remappers={MAPPINGS.actor_data_sound_key: MAPPINGS.sound.shift_up},
    )
    sound.register(p)

    # Override the SoundRewriter's LEVEL_SOUND_EVENT to also append Fire At Position
    p.register_clientbound(PacketId.LEVEL_SOUND_EVENT, rewrite_level_sound_event)

    p.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    p.register_clientbound(PacketId.ACTOR_EVENT, rewrite_actor_event)
    p.register_clientbound(PacketId.PLAY_SOUND, rewrite_play_sound)

    p.register_clientbound(PacketId.PLAYER_EQUIPMENT, rewrite_mob_equipment_clientbound)
    p.register_serverbound(PacketId.PLAYER_EQUIPMENT, rewrite_mob_equipment_serverbound)

    # NetworkItemStackDescriptor -> cerealizer<NetworkItemStackDescriptor>::SerializedData
    p.register_clientbound(PacketId.INVENTORY_SLOT, rewrite_inventory_slot)

    p.register_serverbound(PacketId.UPDATE_CLIENT_OPTIONS, rewrite_update_client_options)
    p.register_serverbound(PacketId.SERVERBOUND_DIAGNOSTICS, rewrite_diagnostics)

    # No in-place mapping (waypoint texture model split, polymorphic shape payload,
    # weight discriminator removed, enchant cost width changed).
    p.cancel_clientbound(
        PacketId.LOCATOR_BAR,  # 341 -- TextureId(int) -> TexturePath(string) + IconSize(Vec2)
        PacketId.SERVER_SCRIPT_DEBUG_DRAWER,  # 328 -- ShapeDataPayload -> PrimitiveShapeDataPayload
        PacketId.CLIENTBOUND_ATTRIBUTE_LAYER_SYNC,  # 345 -- Weight switch removed
        PacketId.PLAYER_ENCHANT_OPTIONS,  # 146 -- ItemEnchantOption.Cost width changed
    )

    # Item-stack carriers whose payload changed in v975. v944 BDS used
    # writeNetItem to serialise items; v975 switched to
    # writeNetworkItemStackDescriptor through a cerealizer wrapper with
    # different field widths and no air shortcut. A first attempt at an
    # InventorySlot rewriter (handlers/item_stack.py) decoded v944 items
    # correctly but produced output the v975 client refused, suggesting the
    # outer InventorySlot layout (Container Id / Slot widths, FullContainerName
    # optionality) or the cerealizer "Net Id Variant: ItemStackNetIdVariant
    # optional" semantics differ from the protocol-docs spec. Cancelling all
    # four lets login complete; rewriters require BDS-header verification.
    p.cancel_clientbound(
        PacketId.CRAFTING_DATA,  # 52
        PacketId.CREATIVE_CONTENT,  # 145
        PacketId.ITEM_REGISTRY,  # 162
    )

    # No safe downgrade
    p.cancel_serverbound(
        PacketId.PARTY_CHANGED,  # 342 -- string -> PlayerPartyInfo struct
    )

    return p
