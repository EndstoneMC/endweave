#include "endweave/protocols/v1001/protocol.h"

#include "endweave/protocol/convert.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace endweave {

namespace bp = bedrock::protocol;

namespace {
constexpr auto kV975 = ProtocolVersion::V975;
constexpr auto kV1001 = ProtocolVersion::V1001;
} // namespace

// Everything below converts between the two eras' forms of one type. A type whose field list
// did not change needs nothing here: Conversion's primary template walks it field by field and
// recurses into whatever nested types did change. Only a genuine reshape earns a
// specialisation, and the field-count static_assert means forgetting one is a compile error
// rather than a silently dropped field.

// --- 345 ClientboundAttributeLayerSync -------------------------------------------------
//
// The 976 diff is confined to two nested types; the rest of the closure is version-shared.

/**
 * Copies every field both versions of EnvironmentAttributeData have.
 *
 * Only 1001 has local_transition_ticks, noise_transition. The converter sets or drops it
 * deliberately.
 */
template <class To, class From>
void copyEnvironmentAttribute(To &to, const From &from)
{
    to.name = from.name;
    to.from_attribute = from.from_attribute;
    to.attribute = from.attribute;
    to.to_attribute = from.to_attribute;
    to.current_transition_ticks = from.current_transition_ticks;
    to.total_transition_ticks = from.total_transition_ticks;
    to.easing = from.easing;
}

/** EnvironmentAttributeData appended local_transition_ticks and noise_transition. */
template <>
struct Conversion<bp::EnvironmentAttributeData_<kV1001>, bp::EnvironmentAttributeData_<kV975>> {
    static bp::EnvironmentAttributeData_<kV1001> apply(const bp::EnvironmentAttributeData_<kV975> &in)
    {
        bp::EnvironmentAttributeData_<kV1001> out;
        copyEnvironmentAttribute(out, in);
        out.local_transition_ticks = 0; // polyfill
        out.noise_transition = false;   // polyfill
        return out;
    }
};

template <>
struct Conversion<bp::EnvironmentAttributeData_<kV975>, bp::EnvironmentAttributeData_<kV1001>> {
    static bp::EnvironmentAttributeData_<kV975> apply(const bp::EnvironmentAttributeData_<kV1001> &in)
    {
        // local_transition_ticks, noise_transition dropped (lossy)
        bp::EnvironmentAttributeData_<kV975> out;
        copyEnvironmentAttribute(out, in);
        return out;
    }
};

/**
 * Copies every field both versions of AttributeLayerData have.
 *
 * Only 1001 has noise_name. The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyAttributeLayer(To &to, const From &from)
{
    to.name = from.name;
    to.dimension_id = from.dimension_id;
    to.settings = from.settings;
    to.attributes = convert<decltype(to.attributes)>(from.attributes);
}

/** AttributeLayerData inserted noise_name after name. */
template <>
struct Conversion<bp::AttributeLayerData_<kV1001>, bp::AttributeLayerData_<kV975>> {
    static bp::AttributeLayerData_<kV1001> apply(const bp::AttributeLayerData_<kV975> &in)
    {
        bp::AttributeLayerData_<kV1001> out;
        copyAttributeLayer(out, in);
        out.noise_name = std::nullopt; // polyfill
        return out;
    }
};

template <>
struct Conversion<bp::AttributeLayerData_<kV975>, bp::AttributeLayerData_<kV1001>> {
    static bp::AttributeLayerData_<kV975> apply(const bp::AttributeLayerData_<kV1001> &in)
    {
        // noise_name dropped (lossy)
        bp::AttributeLayerData_<kV975> out;
        copyAttributeLayer(out, in);
        return out;
    }
};

// --- 315 ServerboundDiagnostics --------------------------------------------------------

/**
 * Copies every field both versions of ServerboundDiagnosticsPacket have.
 *
 * Only 1001 has whisker_scopes. The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyDiagnostics(To &to, const From &from)
{
    to.avg_fps = from.avg_fps;
    to.avg_server_sim_tick_time_ms = from.avg_server_sim_tick_time_ms;
    to.avg_client_sim_tick_time_ms = from.avg_client_sim_tick_time_ms;
    to.avg_begin_frame_time_ms = from.avg_begin_frame_time_ms;
    to.avg_input_time_ms = from.avg_input_time_ms;
    to.avg_render_time_ms = from.avg_render_time_ms;
    to.avg_end_frame_time_ms = from.avg_end_frame_time_ms;
    to.avg_remainder_time_percent = from.avg_remainder_time_percent;
    to.avg_unaccounted_time_percent = from.avg_unaccounted_time_percent;
    to.memory_category_values = from.memory_category_values;
    to.entity_diagnostics = from.entity_diagnostics;
    to.system_diagnostics = from.system_diagnostics;
}

/**
 * The packet appended a whisker_scopes list at 978.
 *
 * Neither direction needs a fixup: going up the list is left empty, a faithful "a 975 client
 * collects none", and going down it is simply not copied (lossy).
 */
template <>
struct Conversion<bp::ServerboundDiagnosticsPacket_<kV1001>, bp::ServerboundDiagnosticsPacket_<kV975>> {
    static bp::ServerboundDiagnosticsPacket_<kV1001> apply(const bp::ServerboundDiagnosticsPacket_<kV975> &in)
    {
        bp::ServerboundDiagnosticsPacket_<kV1001> out;
        copyDiagnostics(out, in);
        return out;
    }
};

template <>
struct Conversion<bp::ServerboundDiagnosticsPacket_<kV975>, bp::ServerboundDiagnosticsPacket_<kV1001>> {
    static bp::ServerboundDiagnosticsPacket_<kV975> apply(const bp::ServerboundDiagnosticsPacket_<kV1001> &in)
    {
        bp::ServerboundDiagnosticsPacket_<kV975> out;
        copyDiagnostics(out, in);
        return out;
    }
};

// --- 175 SubChunkRequest ---------------------------------------------------------------
//
// The 979 cerealisation reorders the packet and reworks how two fields encode, but carries the
// same three values, so neither direction loses anything. SubChunkPos keeps its field list --
// only the encoding moved, varint32 to fixed int32 -- so it needs no converter.

/**
 * Copies every field both versions of SubChunkRequestPacket have.
 */
template <class To, class From>
void copySubChunkRequest(To &to, const From &from)
{
    to.dimension_type = from.dimension_type;
    to.center_pos = convert<decltype(to.center_pos)>(from.center_pos);
    to.sub_chunk_pos_offsets = from.sub_chunk_pos_offsets;
}

/** 979 moved center_pos behind the offsets; matching by name makes the reorder a non-event. */
template <>
struct Conversion<bp::SubChunkRequestPacket_<kV1001>, bp::SubChunkRequestPacket_<kV975>> {
    static bp::SubChunkRequestPacket_<kV1001> apply(const bp::SubChunkRequestPacket_<kV975> &in)
    {
        bp::SubChunkRequestPacket_<kV1001> out;
        copySubChunkRequest(out, in);
        return out;
    }
};

template <>
struct Conversion<bp::SubChunkRequestPacket_<kV975>, bp::SubChunkRequestPacket_<kV1001>> {
    static bp::SubChunkRequestPacket_<kV975> apply(const bp::SubChunkRequestPacket_<kV1001> &in)
    {
        bp::SubChunkRequestPacket_<kV975> out;
        copySubChunkRequest(out, in);
        return out;
    }
};

// --- 347 ServerPresenceInfo, and the join info StartGame carries ------------------------

/** 999 made both names optional and 980 appended rich_presence_id. */
template <>
struct Conversion<bp::PresenceConfiguration_<kV1001>, bp::PresenceConfiguration_<kV975>> {
    static bp::PresenceConfiguration_<kV1001> apply(const bp::PresenceConfiguration_<kV975> &in)
    {
        // An empty id leaves the client driving its own rich presence, which is what a 975
        // server means.
        return {.experience_name = in.experience_name, .world_name = in.world_name, .rich_presence_id = {}};
    }
};

/**
 * Downgrades a presence configuration, or refuses one 975 cannot spell.
 *
 * A 975 name is always on the wire and BDS constrains it to at least one character, so an
 * absent 1001 name has no 975 form -- not even the empty string.
 */
std::optional<bp::PresenceConfiguration_<kV975>> downgradePresence(const bp::PresenceConfiguration_<kV1001> &in)
{
    if (!in.experience_name.has_value() || !in.world_name.has_value()) {
        return std::nullopt;
    }
    // rich_presence_id dropped (lossy)
    return bp::PresenceConfiguration_<kV975>{.experience_name = *in.experience_name, .world_name = *in.world_name};
}

/**
 * An unspellable configuration drops to an absent one rather than inventing a name; the
 * enclosing optional already means "fall back to the default presence".
 */
template <>
struct Conversion<std::optional<bp::PresenceConfiguration_<kV975>>, std::optional<bp::PresenceConfiguration_<kV1001>>> {
    static std::optional<bp::PresenceConfiguration_<kV975>> apply(
        const std::optional<bp::PresenceConfiguration_<kV1001>> &in)
    {
        if (!in.has_value()) {
            return std::nullopt;
        }
        return downgradePresence(*in);
    }
};

// --- 135 ClientCacheBlobStatus ---------------------------------------------------------
//
// 975 writes both counts up front then both arrays; 1001 gives each list its own prefix.

/**
 * Copies every field both versions of ClientCacheBlobStatusPacket have.
 *
 * Only 975 has missing_count, found_count. The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyBlobStatus(To &to, const From &from)
{
    to.missing_ids = from.missing_ids;
    to.found_ids = from.found_ids;
}

template <>
struct Conversion<bp::ClientCacheBlobStatusPacket_<kV1001>, bp::ClientCacheBlobStatusPacket_<kV975>> {
    static bp::ClientCacheBlobStatusPacket_<kV1001> apply(const bp::ClientCacheBlobStatusPacket_<kV975> &in)
    {
        // 1001 prefixes each list instead of carrying the counts up front, so they just go.
        bp::ClientCacheBlobStatusPacket_<kV1001> out;
        copyBlobStatus(out, in);
        return out;
    }
};

template <>
struct Conversion<bp::ClientCacheBlobStatusPacket_<kV975>, bp::ClientCacheBlobStatusPacket_<kV1001>> {
    static bp::ClientCacheBlobStatusPacket_<kV975> apply(const bp::ClientCacheBlobStatusPacket_<kV1001> &in)
    {
        bp::ClientCacheBlobStatusPacket_<kV975> out;
        copyBlobStatus(out, in);
        // 975 states each length up front; 1001 only implies it through the list prefix.
        out.missing_count = static_cast<std::uint32_t>(out.missing_ids.size());
        out.found_count = static_cast<std::uint32_t>(out.found_ids.size());
        return out;
    }
};

// --- 331 GraphicsOverrideParameter -----------------------------------------------------

/**
 * Copies every field both versions of GraphicsOverrideParameterPacket have.
 *
 * Only 1001 has player_id. The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyGraphicsOverride(To &to, const From &from)
{
    to.keyframes = from.keyframes;
    to.float_value = from.float_value;
    to.vec3_value = from.vec3_value;
    to.biome_id = from.biome_id;
    to.parameter_id = from.parameter_id;
    to.reset_parameter = from.reset_parameter;
}

/**
 * player_id was inserted between biome_id and parameter_id.
 *
 * Going up it is left absent -- a 975 override is never per-player -- and going down it is
 * dropped, so a targeted override becomes a broadcast one (lossy). Neither needs a fixup.
 */
template <>
struct Conversion<bp::GraphicsOverrideParameterPacket_<kV1001>, bp::GraphicsOverrideParameterPacket_<kV975>> {
    static bp::GraphicsOverrideParameterPacket_<kV1001> apply(const bp::GraphicsOverrideParameterPacket_<kV975> &in)
    {
        bp::GraphicsOverrideParameterPacket_<kV1001> out;
        copyGraphicsOverride(out, in);
        return out;
    }
};

template <>
struct Conversion<bp::GraphicsOverrideParameterPacket_<kV975>, bp::GraphicsOverrideParameterPacket_<kV1001>> {
    static bp::GraphicsOverrideParameterPacket_<kV975> apply(const bp::GraphicsOverrideParameterPacket_<kV1001> &in)
    {
        bp::GraphicsOverrideParameterPacket_<kV975> out;
        copyGraphicsOverride(out, in);
        return out;
    }
};

// --- 11 StartGame ----------------------------------------------------------------------
//
// One field inserted on the packet and two appended to LevelSettings. Both are wider than the
// field-count ladder, so their shared prefix is spelled out.

/**
 * Copies every field both versions of LevelSettings have.
 *
 * Only 1001 has server_editor_connection_policy, allow_anonymous_block_drops_in_editor_worlds.
 * The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyLevelSettings(To &to, const From &from)
{
    to.seed = from.seed;
    to.spawn_settings = from.spawn_settings;
    to.generator = from.generator;
    to.game_type = from.game_type;
    to.is_hardcore = from.is_hardcore;
    to.game_difficulty = from.game_difficulty;
    to.default_spawn = from.default_spawn;
    to.achievements_disabled = from.achievements_disabled;
    to.editor_world_type = from.editor_world_type;
    to.is_created_in_editor = from.is_created_in_editor;
    to.is_exported_from_editor = from.is_exported_from_editor;
    to.time = from.time;
    to.education_edition_offer = from.education_edition_offer;
    to.education_features_enabled = from.education_features_enabled;
    to.education_product_id = from.education_product_id;
    to.rain_level = from.rain_level;
    to.lightning_level = from.lightning_level;
    to.confirmed_platform_locked_content = from.confirmed_platform_locked_content;
    to.multiplayer_game_intent = from.multiplayer_game_intent;
    to.lan_broadcast_intent = from.lan_broadcast_intent;
    to.xbl_broadcast_intent = from.xbl_broadcast_intent;
    to.platform_broadcast_intent = from.platform_broadcast_intent;
    to.commands_enabled = from.commands_enabled;
    to.texture_packs_required = from.texture_packs_required;
    to.game_rules = from.game_rules;
    to.experiments = from.experiments;
    to.experiments_previously_toggled = from.experiments_previously_toggled;
    to.bonus_chest_enabled = from.bonus_chest_enabled;
    to.start_with_map_enabled = from.start_with_map_enabled;
    to.default_permissions = from.default_permissions;
    to.server_chunk_tick_range = from.server_chunk_tick_range;
    to.has_locked_behavior_pack = from.has_locked_behavior_pack;
    to.has_locked_resource_pack = from.has_locked_resource_pack;
    to.is_from_locked_template = from.is_from_locked_template;
    to.use_msa_gamertags_only = from.use_msa_gamertags_only;
    to.is_from_world_template = from.is_from_world_template;
    to.is_world_template_option_locked = from.is_world_template_option_locked;
    to.spawn_v1_villagers = from.spawn_v1_villagers;
    to.persona_disabled = from.persona_disabled;
    to.custom_skins_disabled = from.custom_skins_disabled;
    to.emote_chat_muted = from.emote_chat_muted;
    to.base_game_version = from.base_game_version;
    to.limited_world_width = from.limited_world_width;
    to.limited_world_depth = from.limited_world_depth;
    to.nether_type = from.nether_type;
    to.edu_shared_uri_resource = from.edu_shared_uri_resource;
    to.override_force_experimental_gameplay = from.override_force_experimental_gameplay;
    to.chat_restriction_level = from.chat_restriction_level;
    to.disable_player_interactions = from.disable_player_interactions;
}

/**
 * Copies every field both versions of StartGamePacket have.
 *
 * Only 1001 has is_chat_logging. The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyStartGame(To &to, const From &from)
{
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.entity_game_type = from.entity_game_type;
    to.pos = from.pos;
    to.rot = from.rot;
    to.settings = convert<decltype(to.settings)>(from.settings);
    to.level_id = from.level_id;
    to.level_name = from.level_name;
    to.template_content_identity = from.template_content_identity;
    to.is_trial = from.is_trial;
    to.movement_settings = from.movement_settings;
    to.level_current_time = from.level_current_time;
    to.enchantment_seed = from.enchantment_seed;
    to.block_properties = from.block_properties;
    to.multiplayer_correlation_id = from.multiplayer_correlation_id;
    to.enable_item_stack_net_manager = from.enable_item_stack_net_manager;
    to.server_version = from.server_version;
    to.player_property_data = from.player_property_data;
    to.server_block_type_registry_checksum = from.server_block_type_registry_checksum;
    to.world_template_id = from.world_template_id;
    to.server_enabled_client_side_generation = from.server_enabled_client_side_generation;
    to.block_network_ids_are_hashes = from.block_network_ids_are_hashes;
    to.network_permissions = from.network_permissions;
    to.server_configuration_join_info =
        convert<decltype(to.server_configuration_join_info)>(from.server_configuration_join_info);
    to.server_telemetry_data = from.server_telemetry_data;
}

/** LevelSettings appended two editor fields at 1001; StartGame inserted is_chat_logging. */
template <>
struct Conversion<bp::LevelSettings_<kV1001>, bp::LevelSettings_<kV975>> {
    static bp::LevelSettings_<kV1001> apply(const bp::LevelSettings_<kV975> &in)
    {
        bp::LevelSettings_<kV1001> out;
        copyLevelSettings(out, in);
        // A 975 server never opts a world into editor connections.
        out.server_editor_connection_policy = bp::ServerEditorConnectionPolicy::MATCH_WORLD_TYPE;
        out.allow_anonymous_block_drops_in_editor_worlds = false;
        return out;
    }
};

template <>
struct Conversion<bp::LevelSettings_<kV975>, bp::LevelSettings_<kV1001>> {
    static bp::LevelSettings_<kV975> apply(const bp::LevelSettings_<kV1001> &in)
    {
        // server_editor_connection_policy, allow_anonymous_block_drops_in_editor_worlds dropped
        bp::LevelSettings_<kV975> out;
        copyLevelSettings(out, in);
        return out;
    }
};

template <>
struct Conversion<bp::StartGamePacket_<kV1001>, bp::StartGamePacket_<kV975>> {
    static bp::StartGamePacket_<kV1001> apply(const bp::StartGamePacket_<kV975> &in)
    {
        // settings and server_configuration_join_info are versioned; the copy recurses.
        bp::StartGamePacket_<kV1001> out;
        copyStartGame(out, in);
        out.is_chat_logging = false; // polyfill: a 975 server never asks the client to log chat
        return out;
    }
};

template <>
struct Conversion<bp::StartGamePacket_<kV975>, bp::StartGamePacket_<kV1001>> {
    static bp::StartGamePacket_<kV975> apply(const bp::StartGamePacket_<kV1001> &in)
    {
        // is_chat_logging dropped (lossy)
        bp::StartGamePacket_<kV975> out;
        copyStartGame(out, in);
        return out;
    }
};

// --- 74 BossEvent ----------------------------------------------------------------------
//
// 984 cerealised the packet: player_id moved ahead of event_type, darken_screen went, and the
// eight switch arms flattened so every field is now written unconditionally.

/**
 * Copies every field both versions of BossEventPacket have.
 *
 * Only 975 has darken_screen. The converter sets or drops it deliberately.
 */
template <class To, class From>
void copyBossEvent(To &to, const From &from)
{
    to.boss_id = from.boss_id;
    to.event_type = from.event_type;
    to.player_id = from.player_id;
    to.name = from.name;
    to.filtered_name = from.filtered_name;
    to.health_percent = from.health_percent;
    to.color = from.color;
    to.overlay = from.overlay;
}

/**
 * 984 cerealised the packet: player_id moved ahead of event_type, darken_screen went, and the
 * eight switch arms flattened so every field is now written unconditionally.
 *
 * The reorder needs no handling -- fields match by name. The 975 form only wrote the fields its
 * event type selected and left the rest default-constructed; 1001 writes them all, so the
 * unselected ones go out as defaults.
 */
template <>
struct Conversion<bp::BossEventPacket_<kV1001>, bp::BossEventPacket_<kV975>> {
    static bp::BossEventPacket_<kV1001> apply(const bp::BossEventPacket_<kV975> &in)
    {
        // darken_screen has nowhere to go (lossy).
        bp::BossEventPacket_<kV1001> out;
        copyBossEvent(out, in);
        return out;
    }
};

template <>
struct Conversion<bp::BossEventPacket_<kV975>, bp::BossEventPacket_<kV1001>> {
    static bp::BossEventPacket_<kV975> apply(const bp::BossEventPacket_<kV1001> &in)
    {
        bp::BossEventPacket_<kV975> out;
        copyBossEvent(out, in);
        out.darken_screen = 0; // polyfill: 1001 carries no value to restore
        return out;
    }
};

// --- 122 BiomeDefinitionList -----------------------------------------------------------
//
// 981 reshaped BiomeNoiseGradientSurfaceData. The four structs between it and the packet keep
// their field lists, so they need no converters.

template <>
struct Conversion<bp::BiomeNoiseGradientSurfaceData_<kV1001>, bp::BiomeNoiseGradientSurfaceData_<kV975>> {
    static bp::BiomeNoiseGradientSurfaceData_<kV1001> apply(const bp::BiomeNoiseGradientSurfaceData_<kV975> &in)
    {
        bp::BiomeNoiseGradientSurfaceData_<kV1001> out;
        out.non_replaceable_blocks = in.non_replaceable_blocks;
        // A flat block id becomes a specifier carrying a noise name, a threshold and a range.
        // 975 has none of those, so each block becomes an unnamed specifier over the default
        // range and the client draws the gradient it would have drawn anyway.
        out.gradient_block_ranges.reserve(in.gradient_blocks.size());
        for (const std::uint32_t block : in.gradient_blocks) {
            out.gradient_block_ranges.push_back({.block_runtime_id = block});
        }
        // The three flat noise fields collapse into one struct -- a rename, so nothing is lost.
        out.noise_descriptor = {
            .name = in.noise_seed_string, .first_octave = in.first_octave, .amplitudes = in.amplitudes};
        return out;
    }
};

template <>
struct Conversion<bp::BiomeNoiseGradientSurfaceData_<kV975>, bp::BiomeNoiseGradientSurfaceData_<kV1001>> {
    static bp::BiomeNoiseGradientSurfaceData_<kV975> apply(const bp::BiomeNoiseGradientSurfaceData_<kV1001> &in)
    {
        bp::BiomeNoiseGradientSurfaceData_<kV975> out;
        out.non_replaceable_blocks = in.non_replaceable_blocks;
        // Each specifier keeps only its block id; noise, threshold and range have no 975 form.
        out.gradient_blocks.reserve(in.gradient_block_ranges.size());
        for (const auto &specifier : in.gradient_block_ranges) {
            out.gradient_blocks.push_back(specifier.block_runtime_id);
        }
        out.noise_seed_string = in.noise_descriptor.name;
        out.first_octave = in.noise_descriptor.first_octave;
        out.amplitudes = in.noise_descriptor.amplitudes;
        return out;
    }
};

// --- 328 PrimitiveShapes ---------------------------------------------------------------
//
// 1001 widened the payload union from six cases to ten and added four shape types. Indices 0-5
// are stable, so the five old payload structs are version-shared.

template <>
struct Conversion<bp::PrimitiveShapeDataPayload_<kV1001>, bp::PrimitiveShapeDataPayload_<kV975>> {
    static bp::PrimitiveShapeDataPayload_<kV1001> apply(const bp::PrimitiveShapeDataPayload_<kV975> &in)
    {
        bp::PrimitiveShapeDataPayload_<kV1001> out;
        out.network_id = in.network_id;
        if (in.shape_type.has_value()) {
            out.shape_type = static_cast<bp::ScriptPrimitiveShapeType_<kV1001>>(*in.shape_type);
        }
        out.location = in.location;
        out.scale = in.scale;
        out.rotation = in.rotation;
        out.time_left_total_sec = in.time_left_total_sec;
        out.max_render_distance = in.max_render_distance;
        out.color = in.color;
        out.dimension_id = in.dimension_id;
        out.attached_to_id = in.attached_to_id;
        // Every 975 case exists at 1001 under the same index.
        std::visit(
            [&out](const auto &payload) {
                out.extra_data_payload = payload;
            },
            in.extra_data_payload);
        return out;
    }
};

/**
 * Downgrades one shape, or refuses it if 1001 gave it a shape 975 cannot draw.
 *
 * shape_type is integer-coded, so the codec would happily write a 6-9 that a 975 client has no
 * renderer for. The shape has to be dropped rather than clamped.
 */
std::optional<bp::PrimitiveShapeDataPayload_<kV975>> downgradeShape(const bp::PrimitiveShapeDataPayload_<kV1001> &in)
{
    using ShapeType = bp::ScriptPrimitiveShapeType_<kV1001>;
    if (in.shape_type.has_value() && *in.shape_type > ShapeType::ARROW) {
        return std::nullopt;
    }
    if (in.extra_data_payload.index() > 5) {
        return std::nullopt;
    }

    bp::PrimitiveShapeDataPayload_<kV975> out;
    out.network_id = in.network_id;
    if (in.shape_type.has_value()) {
        out.shape_type = static_cast<bp::ScriptPrimitiveShapeType_<kV975>>(*in.shape_type);
    }
    out.location = in.location;
    out.scale = in.scale;
    out.rotation = in.rotation;
    out.time_left_total_sec = in.time_left_total_sec;
    out.max_render_distance = in.max_render_distance;
    out.color = in.color;
    out.dimension_id = in.dimension_id;
    out.attached_to_id = in.attached_to_id;
    std::visit(
        [&out](const auto &payload) {
            if constexpr (requires { out.extra_data_payload = payload; }) {
                out.extra_data_payload = payload;
            }
        },
        in.extra_data_payload);
    return out;
}

/** Shapes 975 cannot draw are dropped from the list rather than clamped onto another shape. */
template <>
struct Conversion<bp::PrimitiveShapesPacket_<kV975>, bp::PrimitiveShapesPacket_<kV1001>> {
    static bp::PrimitiveShapesPacket_<kV975> apply(const bp::PrimitiveShapesPacket_<kV1001> &in)
    {
        bp::PrimitiveShapesPacket_<kV975> out;
        out.shapes.reserve(in.shapes.size());
        for (const auto &shape : in.shapes) {
            if (auto converted = downgradeShape(shape)) {
                out.shapes.push_back(*std::move(converted));
            }
        }
        return out;
    }
};

// --- 30, 32, 49: the item-descriptor closure -------------------------------------------
//
// 1001 cerealised SerializedNetworkItemStackDescriptor -- id narrowed to int16, net_id became a
// three-case union, block_runtime_id went unsigned, and the when(id != 0) early-out
// disappeared. Every packet carrying an item stack rides on that.

template <>
struct Conversion<bp::SerializedNetworkItemStackDescriptor_<kV1001>, bp::SerializedNetworkItemStackDescriptor_<kV975>> {
    static bp::SerializedNetworkItemStackDescriptor_<kV1001> apply(
        const bp::SerializedNetworkItemStackDescriptor_<kV975> &in)
    {
        bp::SerializedNetworkItemStackDescriptor_<kV1001> out;
        out.id = static_cast<std::int16_t>(in.id);
        out.stack_size = in.stack_size;
        out.aux_value = in.aux_value;
        if (in.net_id.has_value()) {
            out.net_id_variant = *in.net_id; // 975 only ever carries the plain net id
        }
        out.block_runtime_id = static_cast<std::uint32_t>(in.block_runtime_id);
        out.user_data_buffer = in.user_data_buffer;
        return out;
    }
};

template <>
struct Conversion<bp::SerializedNetworkItemStackDescriptor_<kV975>, bp::SerializedNetworkItemStackDescriptor_<kV1001>> {
    static bp::SerializedNetworkItemStackDescriptor_<kV975> apply(
        const bp::SerializedNetworkItemStackDescriptor_<kV1001> &in)
    {
        bp::SerializedNetworkItemStackDescriptor_<kV975> out;
        out.id = in.id;
        out.stack_size = in.stack_size;
        out.aux_value = in.aux_value;
        if (in.net_id_variant.has_value()) {
            // 975 has only the plain net id. A request id or a legacy request id has no form
            // there, so it drops to absent rather than being passed off as a net id (lossy).
            if (const auto *net_id = std::get_if<bp::ItemStackNetId>(&*in.net_id_variant)) {
                out.net_id = *net_id;
            }
        }
        out.block_runtime_id = static_cast<std::int32_t>(in.block_runtime_id);
        out.user_data_buffer = in.user_data_buffer;
        return out;
    }
};

/** 975 gates container_id and flags on source_type; 1001 makes them real optionals. */
template <>
struct Conversion<bp::InventorySource_<kV1001>, bp::InventorySource_<kV975>> {
    static bp::InventorySource_<kV1001> apply(const bp::InventorySource_<kV975> &in)
    {
        using SourceType = bp::InventorySourceType;
        bp::InventorySource_<kV1001> out;
        out.source_type = in.source_type;
        if (in.source_type == SourceType::CONTAINER_INVENTORY ||
            in.source_type == SourceType::NON_IMPLEMENTED_FEATURE_TODO) {
            out.container_id = in.container_id;
        }
        if (in.source_type == SourceType::WORLD_INTERACTION) {
            out.flags = in.flags;
        }
        return out;
    }
};

template <>
struct Conversion<bp::InventorySource_<kV975>, bp::InventorySource_<kV1001>> {
    static bp::InventorySource_<kV975> apply(const bp::InventorySource_<kV1001> &in)
    {
        // 975 re-derives which of the two it writes from source_type, so a value the predicate
        // would suppress is dropped either way.
        return {.source_type = in.source_type,
                .container_id = in.container_id.value_or(bp::ContainerID{}),
                .flags = in.flags.value_or(bp::InventorySourceFlags{})};
    }
};

/** The action list became an optional at 1001; 975 always writes one. */
template <>
struct Conversion<bp::InventoryTransaction_<kV1001>, bp::InventoryTransaction_<kV975>> {
    static bp::InventoryTransaction_<kV1001> apply(const bp::InventoryTransaction_<kV975> &in)
    {
        return {.actions = convert<std::vector<bp::InventoryAction_<kV1001>>>(in.actions)};
    }
};

template <>
struct Conversion<bp::InventoryTransaction_<kV975>, bp::InventoryTransaction_<kV1001>> {
    static bp::InventoryTransaction_<kV975> apply(const bp::InventoryTransaction_<kV1001> &in)
    {
        if (!in.actions.has_value()) {
            return {};
        }
        return {.actions = convert<std::vector<bp::InventoryAction_<kV975>>>(*in.actions)};
    }
};

/** face went from a signed to an unsigned varint at 1001. */
template <>
struct Conversion<bp::ItemUseInventoryTransaction_<kV1001>, bp::ItemUseInventoryTransaction_<kV975>> {
    static bp::ItemUseInventoryTransaction_<kV1001> apply(const bp::ItemUseInventoryTransaction_<kV975> &in)
    {
        return {.actions = convert<bp::InventoryTransaction_<kV1001>>(in.actions),
                .action_type = in.action_type,
                .trigger_type = in.trigger_type,
                .pos = in.pos,
                .face = static_cast<std::uint32_t>(in.face),
                .slot = in.slot,
                .item = convert<bp::SerializedNetworkItemStackDescriptor_<kV1001>>(in.item),
                .from_pos = in.from_pos,
                .click_pos = in.click_pos,
                .target_block_id = in.target_block_id,
                .client_predicted_result = in.client_predicted_result,
                .client_cooldown_state = in.client_cooldown_state};
    }
};

template <>
struct Conversion<bp::ItemUseInventoryTransaction_<kV975>, bp::ItemUseInventoryTransaction_<kV1001>> {
    static bp::ItemUseInventoryTransaction_<kV975> apply(const bp::ItemUseInventoryTransaction_<kV1001> &in)
    {
        return {.actions = convert<bp::InventoryTransaction_<kV975>>(in.actions),
                .action_type = in.action_type,
                .trigger_type = in.trigger_type,
                .pos = in.pos,
                .face = static_cast<std::int32_t>(in.face),
                .slot = in.slot,
                .item = convert<bp::SerializedNetworkItemStackDescriptor_<kV975>>(in.item),
                .from_pos = in.from_pos,
                .click_pos = in.click_pos,
                .target_block_id = in.target_block_id,
                .client_predicted_result = in.client_predicted_result,
                .client_cooldown_state = in.client_cooldown_state};
    }
};

/** Both the slot list and the transaction became optionals at 1001. */
template <>
struct Conversion<bp::InventoryTransactionPacket_<kV1001>, bp::InventoryTransactionPacket_<kV975>> {
    static bp::InventoryTransactionPacket_<kV1001> apply(const bp::InventoryTransactionPacket_<kV975> &in)
    {
        bp::InventoryTransactionPacket_<kV1001> out;
        out.legacy_request_id = in.legacy_request_id;
        // 975 gates the slot list on legacy_request_id with no presence byte; 1001 makes it a
        // real optional, so the predicate becomes the has_value.
        if (in.legacy_request_id != 0) {
            out.legacy_set_item_slots = in.legacy_set_item_slots;
        }
        out.transaction = convert<bp::TransactionData_<kV1001>>(in.transaction);
        return out;
    }
};

template <>
struct Conversion<bp::InventoryTransactionPacket_<kV975>, bp::InventoryTransactionPacket_<kV1001>> {
    static bp::InventoryTransactionPacket_<kV975> apply(const bp::InventoryTransactionPacket_<kV1001> &in)
    {
        bp::InventoryTransactionPacket_<kV975> out;
        out.legacy_request_id = in.legacy_request_id;
        // 975 re-derives whether it writes the list from legacy_request_id, so a list present
        // without a request id is dropped either way.
        out.legacy_set_item_slots = in.legacy_set_item_slots.value_or(std::vector<bp::LegacySetSlot>{});
        if (in.transaction.has_value()) {
            out.transaction = convert<bp::TransactionData_<kV975>>(*in.transaction);
        }
        return out;
    }
};

namespace {

/**
 * The named entry point PacketHandlers::map needs, since it takes a function pointer rather
 * than a bare convert<To> instantiation.
 */
template <class To, class From>
To convertPacket(const From &in)
{
    return convert<To>(in);
}

} // namespace

template <template <ProtocolVersion> class Packet>
void Protocol<ProtocolVersion::V1001>::registerBothWays(MinecraftPacketIds packet_id)
{
    registerUpgrade(packet_id, PacketHandlers::map<Packet<kV975>>(&convertPacket<Packet<kV1001>, Packet<kV975>>));
    registerDowngrade(packet_id, PacketHandlers::map<Packet<kV1001>>(&convertPacket<Packet<kV975>, Packet<kV1001>>));
}

void Protocol<ProtocolVersion::V1001>::registerPackets()
{
    using Ids = MinecraftPacketIds;

    registerBothWays<bp::StartGamePacket_>(Ids::StartGame);
    registerBothWays<bp::InventoryTransactionPacket_>(Ids::InventoryTransaction);
    registerBothWays<bp::MobArmorEquipmentPacket_>(Ids::MobArmorEquipment);
    registerBothWays<bp::InventoryContentPacket_>(Ids::InventoryContent);
    registerBothWays<bp::BossEventPacket_>(Ids::BossEvent);
    registerBothWays<bp::BiomeDefinitionListPacket_>(Ids::BiomeDefinitionList);
    registerBothWays<bp::ClientCacheBlobStatusPacket_>(Ids::ClientCacheBlobStatus);
    registerBothWays<bp::SubChunkRequestPacket_>(Ids::SubChunkRequest);
    registerBothWays<bp::ServerboundDiagnosticsPacket_>(Ids::ServerboundDiagnostics);
    registerBothWays<bp::PrimitiveShapesPacket_>(Ids::ServerScriptDebugDrawer);
    registerBothWays<bp::GraphicsOverrideParameterPacket_>(Ids::GraphicsParameterOverride);
    registerBothWays<bp::ClientboundAttributeLayerSyncPacket_>(Ids::ClientboundAttributeLayerSync);
    registerBothWays<bp::ServerPresenceInfoPacket_>(Ids::ServerPresenceInfo);

    // Added at the 977 step: a 975 client has no such id, so drop it.
    cancelDowngrade(Ids::ClientboundUpdateSoundData);

    // 123 LevelSoundEvent carries a numeric LevelSoundEvent at 975 and a free-form sound name
    // at 1001, so translating it needs a number-to-name table for both eras. protocol-docs only
    // publishes wire names for 1001 (r26_u3); the 975 dump lists BDS symbol names instead, six
    // values were removed at 1001, and 601 was reused -- it is Undefined at 975 and
    // slime_landing at 1001. Any table bridging the two would be invented, and a wrong sound id
    // is silent corruption. Until the 975 names are available, drop the packet: the sound goes
    // missing rather than the frame being misread.
    cancelUpgrade(Ids::LevelSoundEvent);
    cancelDowngrade(Ids::LevelSoundEvent);
}

} // namespace endweave
