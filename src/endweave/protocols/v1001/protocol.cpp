#include "endweave/protocols/v1001/protocol.h"

#include <optional>
#include <variant>

namespace endweave {
namespace {

namespace bp = bedrock::protocol;

constexpr auto kV975 = ProtocolVersion::V975;
constexpr auto kV1001 = ProtocolVersion::V1001;

using AttributeLayerSyncV975 = bp::ClientboundAttributeLayerSyncPacket_<kV975>;
using AttributeLayerSyncV1001 = bp::ClientboundAttributeLayerSyncPacket_<kV1001>;
using ServerboundDiagnosticsV975 = bp::ServerboundDiagnosticsPacket_<kV975>;
using ServerboundDiagnosticsV1001 = bp::ServerboundDiagnosticsPacket_<kV1001>;
using SubChunkRequestV975 = bp::SubChunkRequestPacket_<kV975>;
using SubChunkRequestV1001 = bp::SubChunkRequestPacket_<kV1001>;
using ServerPresenceInfoV975 = bp::ServerPresenceInfoPacket_<kV975>;
using ServerPresenceInfoV1001 = bp::ServerPresenceInfoPacket_<kV1001>;

// --- 345 ClientboundAttributeLayerSync -------------------------------------------------
//
// The 976 diff is confined to two nested types; the rest of the closure is version-shared.

// EnvironmentAttributeData: gains local_transition_ticks + noise_transition.
bp::EnvironmentAttributeData_<kV1001> up(const bp::EnvironmentAttributeData_<kV975> &in)
{
    bp::EnvironmentAttributeData_<kV1001> out;
    out.name = in.name;
    out.from_attribute = in.from_attribute;
    out.attribute = in.attribute;
    out.to_attribute = in.to_attribute;
    out.current_transition_ticks = in.current_transition_ticks;
    out.total_transition_ticks = in.total_transition_ticks;
    out.easing = in.easing;
    out.local_transition_ticks = 0; // polyfill
    out.noise_transition = false;   // polyfill
    return out;
}

bp::EnvironmentAttributeData_<kV975> down(const bp::EnvironmentAttributeData_<kV1001> &in)
{
    bp::EnvironmentAttributeData_<kV975> out;
    out.name = in.name;
    out.from_attribute = in.from_attribute;
    out.attribute = in.attribute;
    out.to_attribute = in.to_attribute;
    out.current_transition_ticks = in.current_transition_ticks;
    out.total_transition_ticks = in.total_transition_ticks;
    out.easing = in.easing;
    // local_transition_ticks, noise_transition dropped (lossy)
    return out;
}

// AttributeLayerData: gains noise_name.
bp::AttributeLayerData_<kV1001> up(const bp::AttributeLayerData_<kV975> &in)
{
    bp::AttributeLayerData_<kV1001> out;
    out.name = in.name;
    out.noise_name = std::nullopt; // polyfill
    out.dimension_id = in.dimension_id;
    out.settings = in.settings;
    out.attributes.reserve(in.attributes.size());
    for (const auto &attribute : in.attributes) {
        out.attributes.push_back(up(attribute));
    }
    return out;
}

bp::AttributeLayerData_<kV975> down(const bp::AttributeLayerData_<kV1001> &in)
{
    bp::AttributeLayerData_<kV975> out;
    out.name = in.name;
    // noise_name dropped (lossy)
    out.dimension_id = in.dimension_id;
    out.settings = in.settings;
    out.attributes.reserve(in.attributes.size());
    for (const auto &attribute : in.attributes) {
        out.attributes.push_back(down(attribute));
    }
    return out;
}

// Union case 0: UpdateAttributeLayersData.
bp::UpdateAttributeLayersData_<kV1001> up(const bp::UpdateAttributeLayersData_<kV975> &in)
{
    bp::UpdateAttributeLayersData_<kV1001> out;
    out.attribute_layers.reserve(in.attribute_layers.size());
    for (const auto &layer : in.attribute_layers) {
        out.attribute_layers.push_back(up(layer));
    }
    return out;
}

bp::UpdateAttributeLayersData_<kV975> down(const bp::UpdateAttributeLayersData_<kV1001> &in)
{
    bp::UpdateAttributeLayersData_<kV975> out;
    out.attribute_layers.reserve(in.attribute_layers.size());
    for (const auto &layer : in.attribute_layers) {
        out.attribute_layers.push_back(down(layer));
    }
    return out;
}

// Union case 2: UpdateEnvironmentAttributesData.
bp::UpdateEnvironmentAttributesData_<kV1001> up(const bp::UpdateEnvironmentAttributesData_<kV975> &in)
{
    bp::UpdateEnvironmentAttributesData_<kV1001> out;
    out.layer_name = in.layer_name;
    out.layer_dimension_id = in.layer_dimension_id;
    out.attributes.reserve(in.attributes.size());
    for (const auto &attribute : in.attributes) {
        out.attributes.push_back(up(attribute));
    }
    return out;
}

bp::UpdateEnvironmentAttributesData_<kV975> down(const bp::UpdateEnvironmentAttributesData_<kV1001> &in)
{
    bp::UpdateEnvironmentAttributesData_<kV975> out;
    out.layer_name = in.layer_name;
    out.layer_dimension_id = in.layer_dimension_id;
    out.attributes.reserve(in.attributes.size());
    for (const auto &attribute : in.attributes) {
        out.attributes.push_back(down(attribute));
    }
    return out;
}

AttributeLayerSyncV1001 upgrade(const AttributeLayerSyncV975 &packet)
{
    AttributeLayerSyncV1001 out;
    switch (packet.data.index()) {
    case 0:
        out.data = up(std::get<0>(packet.data));
        break;
    case 1:
        out.data = std::get<1>(packet.data); // version-shared
        break;
    case 2:
        out.data = up(std::get<2>(packet.data));
        break;
    case 3:
        out.data = std::get<3>(packet.data); // version-shared
        break;
    }
    return out;
}

AttributeLayerSyncV975 downgrade(const AttributeLayerSyncV1001 &packet)
{
    AttributeLayerSyncV975 out;
    switch (packet.data.index()) {
    case 0:
        out.data = down(std::get<0>(packet.data));
        break;
    case 1:
        out.data = std::get<1>(packet.data); // version-shared
        break;
    case 2:
        out.data = down(std::get<2>(packet.data));
        break;
    case 3:
        out.data = std::get<3>(packet.data); // version-shared
        break;
    }
    return out;
}

// --- 315 ServerboundDiagnostics --------------------------------------------------------
//
// The 978 diff is one trailing list; every other field is version-shared.

ServerboundDiagnosticsV1001 upgrade(const ServerboundDiagnosticsV975 &packet)
{
    ServerboundDiagnosticsV1001 out;
    out.avg_fps = packet.avg_fps;
    out.avg_server_sim_tick_time_ms = packet.avg_server_sim_tick_time_ms;
    out.avg_client_sim_tick_time_ms = packet.avg_client_sim_tick_time_ms;
    out.avg_begin_frame_time_ms = packet.avg_begin_frame_time_ms;
    out.avg_input_time_ms = packet.avg_input_time_ms;
    out.avg_render_time_ms = packet.avg_render_time_ms;
    out.avg_end_frame_time_ms = packet.avg_end_frame_time_ms;
    out.avg_remainder_time_percent = packet.avg_remainder_time_percent;
    out.avg_unaccounted_time_percent = packet.avg_unaccounted_time_percent;
    out.memory_category_values = packet.memory_category_values;
    out.entity_diagnostics = packet.entity_diagnostics;
    out.system_diagnostics = packet.system_diagnostics;
    // whisker_scopes polyfilled empty: a 975 client reports no whisker scopes, and the list is
    // length-prefixed, so an empty one is a faithful "none collected".
    out.whisker_scopes = {};
    return out;
}

ServerboundDiagnosticsV975 downgrade(const ServerboundDiagnosticsV1001 &packet)
{
    ServerboundDiagnosticsV975 out;
    out.avg_fps = packet.avg_fps;
    out.avg_server_sim_tick_time_ms = packet.avg_server_sim_tick_time_ms;
    out.avg_client_sim_tick_time_ms = packet.avg_client_sim_tick_time_ms;
    out.avg_begin_frame_time_ms = packet.avg_begin_frame_time_ms;
    out.avg_input_time_ms = packet.avg_input_time_ms;
    out.avg_render_time_ms = packet.avg_render_time_ms;
    out.avg_end_frame_time_ms = packet.avg_end_frame_time_ms;
    out.avg_remainder_time_percent = packet.avg_remainder_time_percent;
    out.avg_unaccounted_time_percent = packet.avg_unaccounted_time_percent;
    out.memory_category_values = packet.memory_category_values;
    out.entity_diagnostics = packet.entity_diagnostics;
    out.system_diagnostics = packet.system_diagnostics;
    // whisker_scopes dropped (lossy)
    return out;
}

// --- 175 SubChunkRequest ---------------------------------------------------------------
//
// The 979 cerealisation reorders the packet and reworks how two fields encode, but carries the
// same three values, so neither direction loses anything.

// SubChunkPos: varint32 x/y/z at 975, fixed int32 at 1001 -- same values, so the C++ layouts
// match and only the type changes.
bp::SubChunkPos_<kV1001> up(const bp::SubChunkPos_<kV975> &in)
{
    return {.x = in.x, .y = in.y, .z = in.z};
}

bp::SubChunkPos_<kV975> down(const bp::SubChunkPos_<kV1001> &in)
{
    return {.x = in.x, .y = in.y, .z = in.z};
}

SubChunkRequestV1001 upgrade(const SubChunkRequestV975 &packet)
{
    SubChunkRequestV1001 out;
    out.dimension_type = packet.dimension_type;
    out.sub_chunk_pos_offsets = packet.sub_chunk_pos_offsets;
    out.center_pos = up(packet.center_pos);
    return out;
}

SubChunkRequestV975 downgrade(const SubChunkRequestV1001 &packet)
{
    SubChunkRequestV975 out;
    out.dimension_type = packet.dimension_type;
    out.center_pos = down(packet.center_pos);
    out.sub_chunk_pos_offsets = packet.sub_chunk_pos_offsets;
    return out;
}

// --- 347 ServerPresenceInfo ------------------------------------------------------------

// PresenceConfiguration: both names become optional (999) and rich_presence_id is appended
// (980). A 975 name is always present, so it upgrades to a present one.
bp::PresenceConfiguration_<kV1001> up(const bp::PresenceConfiguration_<kV975> &in)
{
    bp::PresenceConfiguration_<kV1001> out;
    out.experience_name = in.experience_name;
    out.world_name = in.world_name;
    // Empty polyfill: the id overrides the client-driven rich presence, so empty leaves the
    // client driving it, which is what a 975 server means.
    out.rich_presence_id = {};
    return out;
}

// A 975 name is always on the wire and BDS constrains it to at least one character, so an
// absent 1001 name has no 975 spelling -- not even the empty string.
std::optional<bp::PresenceConfiguration_<kV975>> down(const bp::PresenceConfiguration_<kV1001> &in)
{
    if (!in.experience_name.has_value() || !in.world_name.has_value()) {
        return std::nullopt;
    }
    // rich_presence_id dropped (lossy)
    return bp::PresenceConfiguration_<kV975>{.experience_name = *in.experience_name, .world_name = *in.world_name};
}

ServerPresenceInfoV1001 upgrade(const ServerPresenceInfoV975 &packet)
{
    ServerPresenceInfoV1001 out;
    if (packet.presence_configuration.has_value()) {
        out.presence_configuration = up(*packet.presence_configuration);
    }
    return out;
}

ServerPresenceInfoV975 downgrade(const ServerPresenceInfoV1001 &packet)
{
    ServerPresenceInfoV975 out;
    if (packet.presence_configuration.has_value()) {
        // A config with no 975 spelling drops to an absent one rather than inventing a name;
        // the packet-level optional already means "fall back to the default presence".
        out.presence_configuration = down(*packet.presence_configuration);
    }
    return out;
}

} // namespace

void Protocol<ProtocolVersion::V1001>::registerPackets()
{
    registerUpgrade(MinecraftPacketIds::ClientboundAttributeLayerSync,
                    PacketHandlers::map<AttributeLayerSyncV975>(&upgrade));
    registerDowngrade(MinecraftPacketIds::ClientboundAttributeLayerSync,
                      PacketHandlers::map<AttributeLayerSyncV1001>(&downgrade));

    registerUpgrade(MinecraftPacketIds::ServerboundDiagnostics,
                    PacketHandlers::map<ServerboundDiagnosticsV975>(&upgrade));
    registerDowngrade(MinecraftPacketIds::ServerboundDiagnostics,
                      PacketHandlers::map<ServerboundDiagnosticsV1001>(&downgrade));

    registerUpgrade(MinecraftPacketIds::SubChunkRequest, PacketHandlers::map<SubChunkRequestV975>(&upgrade));
    registerDowngrade(MinecraftPacketIds::SubChunkRequest, PacketHandlers::map<SubChunkRequestV1001>(&downgrade));

    registerUpgrade(MinecraftPacketIds::ServerPresenceInfo, PacketHandlers::map<ServerPresenceInfoV975>(&upgrade));
    registerDowngrade(MinecraftPacketIds::ServerPresenceInfo, PacketHandlers::map<ServerPresenceInfoV1001>(&downgrade));

    // Added at the 977 step: a 975 client has no such id, so drop it.
    cancelDowngrade(MinecraftPacketIds::ClientboundUpdateSoundData);
}

} // namespace endweave
