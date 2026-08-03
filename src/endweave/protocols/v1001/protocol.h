#pragma once

#include "endweave/protocol/version_node.h"
#include "transform.h"

namespace endweave {

/**
 * Protocol 1001 (1.26.30). Owns both directions of the wire diff with the version before it.
 *
 * 1001 is a cerealisation era, so nearly every modelled packet was reshaped and none has a
 * converter yet. Replacing an entry with one is what the translation work amounts to.
 *
 * @see A ViaVersion forward protocol (e.g. Protocol1_20To1_20_2) fused with its ViaBackwards
 * backward protocol (Protocol1_20_2To1_20).
 */
template <>
class Protocol<ProtocolVersion::v26_30> final
    : public VersionNode<ProtocolVersion::v26_30, Protocol<ProtocolVersion::v26_30>> {
    using Ids = bedrock::protocol::MinecraftPacketIds;

    // A reshape needs converting both ways, so both steps start from the same list.
    using Reshaped = Mappings<unconverted<
        Ids::START_GAME, Ids::INVENTORY_TRANSACTION, Ids::MOB_ARMOR_EQUIPMENT, Ids::INVENTORY_CONTENT, Ids::BOSS_EVENT,
        Ids::BIOME_DEFINITION_LIST, Ids::LEVEL_SOUND_EVENT, Ids::CLIENT_CACHE_BLOB_STATUS_PACKET,
        Ids::PACKET_VIOLATION_WARNING, Ids::SUB_CHUNK_REQUEST_PACKET, Ids::SERVERBOUND_DIAGNOSTICS_PACKET,
        Ids::PRIMITIVE_SHAPES_PACKET, Ids::GRAPHICS_PARAMETER_OVERRIDE_PACKET,
        Ids::CLIENTBOUND_ATTRIBUTE_LAYER_SYNC_PACKET, Ids::SERVER_PRESENCE_INFO>>;

public:
    using Upgrades = Reshaped;
    // 975 has no sound-data packet, so there is nothing to hand a 975 client.
    using Downgrades = Reshaped::And<cancel<Ids::CLIENTBOUND_UPDATE_SOUND_DATA>>;
};

} // namespace endweave
