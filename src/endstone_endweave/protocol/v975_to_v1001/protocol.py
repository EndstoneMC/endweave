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

from endstone_endweave.protocol import Protocol
from endstone_endweave.protocol.packet_ids import PacketId

from .handlers.boss_event import rewrite_boss_event, rewrite_boss_event_serverbound
from .handlers.biome_definition_list import rewrite_biome_definition_list
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
    # Clientbound: upgrade StartGame to v1001 wire (adds serverEditorConnectionPolicy,
    # allowAnonymousBlockDropsInEditorWorlds, isLoggingChat) so 1.26.30 clients parse it.
    p.register_clientbound(PacketId.START_GAME, rewrite_start_game)
    # Clientbound: BiomeDefinitionList(122) restructured its nested chunk-gen body at v1001
    # -- the inline surface span moved into a new Optional[SurfaceBuilder] tail (plus an
    # absent-from-v975 NoiseGradientSurface and SubsurfaceBuilder). It is sent unconditionally
    # in the join burst right after StartGame, so an untranslated v975-shaped list makes a
    # 1.26.30 client mis-parse the first biome and drop ~1s after connect.
    p.register_clientbound(PacketId.BIOME_DEFINITION_LIST, rewrite_biome_definition_list)
    # Clientbound: InventoryContent(49) and MobArmourEquipment(32) flipped their embedded
    # item descriptors from the OLD (ItemInstance) to the NEW (ItemInstanceNew) format at
    # v1001 only (they were still OLD at v975). Upgrade them here so geared 1.26.30 clients
    # parse the full-inventory and worn-armor packets they receive at spawn and stop
    # dropping. The item descriptor itself is identical v975<->v1001, so no v1001 item type
    # is needed -- ITEM_INSTANCE_V975 is the correct NEW encoding.
    p.register_clientbound(PacketId.INVENTORY_CONTENT, rewrite_inventory_content)
    p.register_clientbound(PacketId.MOB_ARMOR_EQUIPMENT, rewrite_mob_armour_equipment)
    # Clientbound: LevelSoundEvent(123) changed its first field at v1001 -- SoundType
    # flipped from a numeric enum id (Varuint32) to a lowercase dotted sound-name String;
    # fields 2-8 are byte-identical. The v944_to_v975 layer already remapped the numeric
    # id to v975 numbering, so this handler maps that v975 numeric id -> v1001 wire string
    # and rewrites the field. A melee hit is broadcast as this packet (id 1 = "hit"), so an
    # untranslated v975-numeric SoundType makes a 1.26.30 client read the number as a
    # length-prefixed string, desync, and disconnect cleanly on attack -- the crash this fixes.
    p.register_clientbound(PacketId.LEVEL_SOUND_EVENT, rewrite_level_sound_event)
    # Clientbound: BossEvent(74) flattened its switch-based v975 wire into a fixed v1001
    # layout -- PlayerUniqueID hoisted to a fixed position, ScreenDarkening dropped, and
    # EventType/Colour/Overlay narrowed Varuint32 -> Uint8. Content-CONDITIONAL (only with a
    # boss bar): a content-heavier ~944 server that shows a bar sends the v975 switch shape,
    # and untranslated a 1.26.30 client mis-parses it and drops ~1s after connect. This
    # handler flattens v975 -> v1001 for every event type. Validated offline with gophertunnel
    # (v1.56.2 marshal -> handler -> v1.57.0 decode: 0 leftover, field-equivalent, all 9 types).
    p.register_clientbound(PacketId.BOSS_EVENT, rewrite_boss_event)
    # Clientbound: LevelChunk(58 / FullChunkData) -- rewrite sub-chunk-request "Limited"
    # mode into "Limitless". The v944/v975 server sends SubChunkCount=Limited(-2) followed
    # by a uint16 HighestSubChunk cap computed for the OLD world height; a 1.26.30 client
    # honours that cap and treats every sub-chunk above it as air, so the TOP sections of
    # every column vanish. Rewriting to Limitless(-1) (drop HighestSubChunk) makes the
    # client request the full column so the server fills all sections. Already-Limitless and
    # legacy explicit-count packets pass through byte-for-byte. Validated against 579 real
    # captures (306 cache + 273 non-cache) with gophertunnel v1.57.0: every Limited input
    # becomes Limitless, all other fields byte-identical, converted output exactly 2 bytes
    # shorter (the dropped uint16).
    p.register_clientbound(PacketId.FULL_CHUNK_DATA, rewrite_level_chunk)
    # AvailableCommands(76) deliberately NOT registered: the arg-type id "change" is a
    # gophertunnel iota artifact, not a wire change, and CommandParameter.Type is a
    # fixed-width uint32 that cannot desync a parse -- so it is not the drop cause. Left
    # unhandled pending byte-capture confirmation of the real drop packet.
    # Serverbound: InventoryTransaction(30) gained presence bools, a rewritten
    # InventoryAction source encoding (nested bools + signed int8 WindowID), narrowed
    # scalars, and NEW item descriptors at v1001. Convert the v1001 (NEW) wire back to
    # the v975 (OLD) wire so item interactions reach the v944/v975 server.
    p.register_serverbound(PacketId.INVENTORY_TRANSACTION, rewrite_inventory_transaction)
    # Clientbound: the v944 server also SENDS InventoryTransaction(30) (e.g. a NormalTransaction
    # during join); the v975-shaped wire desyncs a 1.26.30 client ("expected presence bool to be
    # true" -- it reads the actions count as the new hasType bool). Convert OLD -> NEW so the
    # client parses it. Confirmed via offline gophertunnel v1.57.0 validation to be THE drop cause.
    p.register_clientbound(PacketId.INVENTORY_TRANSACTION, rewrite_inventory_transaction_clientbound)
    # Serverbound: ClientCacheBlobStatus(135) reordered its fields at v1001 (counts-first ->
    # count-interleaved-with-array). A 1.26.30 client sends it after spawn during chunk-cache
    # exchange; untranslated, the v944 server reads a miss-hash as HitCount and overflows
    # (PACKET_MALFORMED). Reorder NEW -> OLD so the server parses it.
    p.register_serverbound(PacketId.CLIENT_CACHE_BLOB_STATUS, rewrite_client_cache_blob_status)
    # Serverbound: SubChunkRequest(175) reordered at v1001 -- Position moved after Offsets, its
    # coords switched varint32 -> fixed int32 LE, and the Offsets length prefix narrowed
    # uint32 -> varuint32. Sent during chunk streaming right after spawn; reorder NEW -> OLD.
    p.register_serverbound(PacketId.SUB_CHUNK_REQUEST, rewrite_sub_chunk_request)
    # Serverbound: BossEvent(74) is BIDIRECTIONAL -- a 1.26.30 client sends it (RegisterPlayer/
    # UnregisterPlayer/Request when it sees a boss entity) in the v1001 flat shape, which the
    # v944/v975 server reads as the v975 switch shape, fails ("readNoHeader failed! packetId:
    # 74"), and drops with PACKET_MALFORMED. Convert the v1001 flat wire back to the v975 switch
    # so the server parses it -- the exact inverse of the clientbound flatten above.
    p.register_serverbound(PacketId.BOSS_EVENT, rewrite_boss_event_serverbound)
    # Serverbound: drop the v1001-only PartyDestinationCookieResponse for the older server.
    p.cancel_serverbound(PARTY_DESTINATION_COOKIE_RESPONSE)

    return p
