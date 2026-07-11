#pragma once

#include <bedrock/protocol.hpp>

namespace endweave {

// The two protocol snapshots this MVP bridges. The wire diff introduced at the
// 976 step lives entirely in AttributeLayerData (noise_name) and
// EnvironmentAttributeData (local_transition_ticks, noise_transition).
using AttributeLayerSyncV975 =
    bedrock::protocol::ClientboundAttributeLayerSyncPacket_<bedrock::protocol::ProtocolVersion::V975>;
using AttributeLayerSyncV1001 =
    bedrock::protocol::ClientboundAttributeLayerSyncPacket_<bedrock::protocol::ProtocolVersion::V1001>;

// 975 -> 1001 (old client, new server). New fields are polyfilled with neutral defaults.
AttributeLayerSyncV1001 upgrade(const AttributeLayerSyncV975 &packet);

// 1001 -> 975 (new client, old server). New fields are dropped (lossy).
AttributeLayerSyncV975 downgrade(const AttributeLayerSyncV1001 &packet);

}  // namespace endweave
