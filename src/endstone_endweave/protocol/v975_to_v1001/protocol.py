"""Protocol factory: v975 server <- v1001 client (MC 1.26.20 <- 1.26.30 "Chaos Cubed").

1.26.30 is NOT content-only: it changed the wire format of several packets
(confirmed against CloudburstMC/Protocol, gophertunnel, and Mojang/bedrock-protocol-docs
r/26_u3). The spawn-critical one is StartGamePacket, which gained 3 new fields a
v944/v975-shaped packet lacks -- so a 1.26.30 client desyncs reading it and drops at
spawn. This delta upgrades StartGame to the v1001 wire format and cancels the new
serverbound packet the older server has no handler for.

InventoryContent(49) and MobArmourEquipment(32) also changed at v1001: their embedded
item descriptors flipped from the OLD ItemInstance encoding (still used at v944/v975) to
the NEW ItemInstanceNew/cerealizer encoding. These are the geared-spawn packets (full
inventory + worn armor), so a geared 1.26.30 client drops at spawn without the upgrade.
They are handled here (NOT in v944_to_v975) because the change is target-version specific:
a real v975 client still expects the OLD encoding for these two packets, so upgrading them
upstream would corrupt v975-terminal connections.

InventoryTransaction(30) is SERVERBOUND and also changed at v1001: it gained three
presence bools (hasLegacy/hasType/hasActions), rewrote the InventoryAction source
encoding into nested presence bools with a SIGNED int8 WindowID, narrowed several
scalars, and flipped its embedded item descriptors OLD -> NEW. The serverbound
handler here reads the v1001 (NEW) wire and writes the v975 (OLD) wire so a 1.26.30
client's item interactions (block place / item use / eat / attack) reach the
v944/v975 server correctly. It is forwarded (not cancelled); the v944_to_v975 layer
registers no handler for it because the packet is byte-identical at v944 and v975.

BiomeDefinitionList(122) is sent unconditionally in the join burst (right after
StartGame) and restructured its nested BiomeChunkGeneration at v1001 -- the inline
surface span moved into a new Optional[SurfaceBuilder] tail. Untranslated it makes a
1.26.30 client mis-parse the first biome and drop ~1s after connect; the clientbound
handler here relocates the span into the v1001 layout.

LevelSoundEvent(123, Varuint32->String) is content-CONDITIONAL (only when a sound
plays) so it is absent from the unconditional join burst, but a melee hit broadcasts
it with the attack-family SoundType (id 1 = "hit"); untranslated it crashed 1.26.30
clients on attack, so its clientbound rewrite IS registered here.

The remaining content-CONDITIONAL wire change -- BossEvent(74, only with a boss bar)
-- is not in the join burst; add it if it is observed to cause drops.
"""

from functools import partial

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.direction import Direction
from endstone_endweave.protocol.packet_ids import PacketId

from .handlers.boss_event import rewrite_boss_event, rewrite_boss_event_serverbound
from .handlers.diagnostics import rewrite_diagnostics
from .handlers.client_cache_blob_status import rewrite_client_cache_blob_status
from .handlers.sub_chunk_request import rewrite_sub_chunk_request
from .handlers.inventory_transaction import (
    rewrite_inventory_transaction,
    rewrite_inventory_transaction_clientbound,
)
from .handlers.item_stack import (
    rewrite_inventory_content,
    rewrite_mob_armour_equipment,
)
from .handlers.level_chunk import rewrite_level_chunk
from .handlers.level_sound_event import rewrite_level_sound_event
from .handlers.start_game import rewrite_start_game

SERVER_PROTOCOL = 975
CLIENT_PROTOCOL = 1001

# New serverbound packet introduced in v1001 (CloudburstMC id 350); the v944/v975
# server has no handler for it, so it must not be forwarded.
PARTY_DESTINATION_COOKIE_RESPONSE = 350


def create_protocol() -> Protocol:
    """Create the v975 server <- v1001 client delta protocol."""
    p = Protocol(server_protocol=SERVER_PROTOCOL, client_protocol=CLIENT_PROTOCOL, name="v975_to_v1001")

    p.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    p.register_clientbound(PacketId.INVENTORY_CONTENT, rewrite_inventory_content)
    p.register_clientbound(PacketId.MOB_ARMOR_EQUIPMENT, rewrite_mob_armour_equipment)
    p.register_clientbound(PacketId.LEVEL_SOUND_EVENT, partial(rewrite_level_sound_event, direction=Direction.CLIENTBOUND))
    p.register_clientbound(PacketId.BOSS_EVENT, rewrite_boss_event)
    p.register_clientbound(PacketId.FULL_CHUNK_DATA, rewrite_level_chunk)
    p.register_clientbound(PacketId.INVENTORY_TRANSACTION, rewrite_inventory_transaction_clientbound)

    p.register_serverbound(PacketId.LEVEL_SOUND_EVENT, partial(rewrite_level_sound_event, direction=Direction.SERVERBOUND))
    p.register_serverbound(PacketId.INVENTORY_TRANSACTION, rewrite_inventory_transaction)
    p.register_serverbound(PacketId.CLIENT_CACHE_BLOB_STATUS, rewrite_client_cache_blob_status)
    p.register_serverbound(PacketId.SUB_CHUNK_REQUEST, rewrite_sub_chunk_request)
    p.register_serverbound(PacketId.BOSS_EVENT, rewrite_boss_event_serverbound)
    p.register_serverbound(PacketId.SERVERBOUND_DIAGNOSTICS, rewrite_diagnostics)
    p.cancel_serverbound(PARTY_DESTINATION_COOKIE_RESPONSE)

    return p
