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

/** EnvironmentAttributeData gained local_transition_ticks and noise_transition. */
template <>
struct Conversion<bp::EnvironmentAttributeData_<kV1001>, bp::EnvironmentAttributeData_<kV975>> {
    static bp::EnvironmentAttributeData_<kV1001> apply(const bp::EnvironmentAttributeData_<kV975> &in)
    {
        return {.name = in.name,
                .from_attribute = in.from_attribute,
                .attribute = in.attribute,
                .to_attribute = in.to_attribute,
                .current_transition_ticks = in.current_transition_ticks,
                .total_transition_ticks = in.total_transition_ticks,
                .easing = in.easing,
                .local_transition_ticks = 0, // polyfill
                .noise_transition = false};  // polyfill
    }
};

template <>
struct Conversion<bp::EnvironmentAttributeData_<kV975>, bp::EnvironmentAttributeData_<kV1001>> {
    static bp::EnvironmentAttributeData_<kV975> apply(const bp::EnvironmentAttributeData_<kV1001> &in)
    {
        // local_transition_ticks, noise_transition dropped (lossy)
        return {.name = in.name,
                .from_attribute = in.from_attribute,
                .attribute = in.attribute,
                .to_attribute = in.to_attribute,
                .current_transition_ticks = in.current_transition_ticks,
                .total_transition_ticks = in.total_transition_ticks,
                .easing = in.easing};
    }
};

/** AttributeLayerData gained noise_name, inserted after name. */
template <>
struct Conversion<bp::AttributeLayerData_<kV1001>, bp::AttributeLayerData_<kV975>> {
    static bp::AttributeLayerData_<kV1001> apply(const bp::AttributeLayerData_<kV975> &in)
    {
        return {.name = in.name,
                .noise_name = std::nullopt, // polyfill
                .dimension_id = in.dimension_id,
                .settings = in.settings,
                .attributes = convert<std::vector<bp::EnvironmentAttributeData_<kV1001>>>(in.attributes)};
    }
};

template <>
struct Conversion<bp::AttributeLayerData_<kV975>, bp::AttributeLayerData_<kV1001>> {
    static bp::AttributeLayerData_<kV975> apply(const bp::AttributeLayerData_<kV1001> &in)
    {
        // noise_name dropped (lossy)
        return {.name = in.name,
                .dimension_id = in.dimension_id,
                .settings = in.settings,
                .attributes = convert<std::vector<bp::EnvironmentAttributeData_<kV975>>>(in.attributes)};
    }
};

// --- 315 ServerboundDiagnostics --------------------------------------------------------

/** The packet gained a trailing whisker_scopes list at 978. */
template <>
struct Conversion<bp::ServerboundDiagnosticsPacket_<kV1001>, bp::ServerboundDiagnosticsPacket_<kV975>> {
    static bp::ServerboundDiagnosticsPacket_<kV1001> apply(const bp::ServerboundDiagnosticsPacket_<kV975> &in)
    {
        return {.avg_fps = in.avg_fps,
                .avg_server_sim_tick_time_ms = in.avg_server_sim_tick_time_ms,
                .avg_client_sim_tick_time_ms = in.avg_client_sim_tick_time_ms,
                .avg_begin_frame_time_ms = in.avg_begin_frame_time_ms,
                .avg_input_time_ms = in.avg_input_time_ms,
                .avg_render_time_ms = in.avg_render_time_ms,
                .avg_end_frame_time_ms = in.avg_end_frame_time_ms,
                .avg_remainder_time_percent = in.avg_remainder_time_percent,
                .avg_unaccounted_time_percent = in.avg_unaccounted_time_percent,
                .memory_category_values = in.memory_category_values,
                .entity_diagnostics = in.entity_diagnostics,
                .system_diagnostics = in.system_diagnostics,
                // A 975 client collects no whisker scopes, and the list is length-prefixed, so
                // an empty one is a faithful "none collected".
                .whisker_scopes = {}};
    }
};

template <>
struct Conversion<bp::ServerboundDiagnosticsPacket_<kV975>, bp::ServerboundDiagnosticsPacket_<kV1001>> {
    static bp::ServerboundDiagnosticsPacket_<kV975> apply(const bp::ServerboundDiagnosticsPacket_<kV1001> &in)
    {
        // whisker_scopes dropped (lossy)
        return {.avg_fps = in.avg_fps,
                .avg_server_sim_tick_time_ms = in.avg_server_sim_tick_time_ms,
                .avg_client_sim_tick_time_ms = in.avg_client_sim_tick_time_ms,
                .avg_begin_frame_time_ms = in.avg_begin_frame_time_ms,
                .avg_input_time_ms = in.avg_input_time_ms,
                .avg_render_time_ms = in.avg_render_time_ms,
                .avg_end_frame_time_ms = in.avg_end_frame_time_ms,
                .avg_remainder_time_percent = in.avg_remainder_time_percent,
                .avg_unaccounted_time_percent = in.avg_unaccounted_time_percent,
                .memory_category_values = in.memory_category_values,
                .entity_diagnostics = in.entity_diagnostics,
                .system_diagnostics = in.system_diagnostics};
    }
};

// --- 175 SubChunkRequest ---------------------------------------------------------------
//
// The 979 cerealisation reorders the packet and reworks how two fields encode, but carries the
// same three values, so neither direction loses anything. SubChunkPos keeps its field list --
// only the encoding moved, varint32 to fixed int32 -- so it needs no converter.

template <>
struct Conversion<bp::SubChunkRequestPacket_<kV1001>, bp::SubChunkRequestPacket_<kV975>> {
    static bp::SubChunkRequestPacket_<kV1001> apply(const bp::SubChunkRequestPacket_<kV975> &in)
    {
        return {.dimension_type = in.dimension_type,
                .sub_chunk_pos_offsets = in.sub_chunk_pos_offsets,
                .center_pos = convert<bp::SubChunkPos_<kV1001>>(in.center_pos)};
    }
};

template <>
struct Conversion<bp::SubChunkRequestPacket_<kV975>, bp::SubChunkRequestPacket_<kV1001>> {
    static bp::SubChunkRequestPacket_<kV975> apply(const bp::SubChunkRequestPacket_<kV1001> &in)
    {
        return {.dimension_type = in.dimension_type,
                .center_pos = convert<bp::SubChunkPos_<kV975>>(in.center_pos),
                .sub_chunk_pos_offsets = in.sub_chunk_pos_offsets};
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

template <>
struct Conversion<bp::ClientCacheBlobStatusPacket_<kV1001>, bp::ClientCacheBlobStatusPacket_<kV975>> {
    static bp::ClientCacheBlobStatusPacket_<kV1001> apply(const bp::ClientCacheBlobStatusPacket_<kV975> &in)
    {
        return {.missing_ids = in.missing_ids, .found_ids = in.found_ids};
    }
};

template <>
struct Conversion<bp::ClientCacheBlobStatusPacket_<kV975>, bp::ClientCacheBlobStatusPacket_<kV1001>> {
    static bp::ClientCacheBlobStatusPacket_<kV975> apply(const bp::ClientCacheBlobStatusPacket_<kV1001> &in)
    {
        return {.missing_count = static_cast<std::uint32_t>(in.missing_ids.size()),
                .found_count = static_cast<std::uint32_t>(in.found_ids.size()),
                .missing_ids = in.missing_ids,
                .found_ids = in.found_ids};
    }
};

// --- 331 GraphicsOverrideParameter -----------------------------------------------------

template <>
struct Conversion<bp::GraphicsOverrideParameterPacket_<kV1001>, bp::GraphicsOverrideParameterPacket_<kV975>> {
    static bp::GraphicsOverrideParameterPacket_<kV1001> apply(const bp::GraphicsOverrideParameterPacket_<kV975> &in)
    {
        return {.keyframes = in.keyframes,
                .float_value = in.float_value,
                .vec3_value = in.vec3_value,
                .biome_id = in.biome_id,
                .player_id = std::nullopt, // polyfill: a 975 override is never per-player
                .parameter_id = in.parameter_id,
                .reset_parameter = in.reset_parameter};
    }
};

template <>
struct Conversion<bp::GraphicsOverrideParameterPacket_<kV975>, bp::GraphicsOverrideParameterPacket_<kV1001>> {
    static bp::GraphicsOverrideParameterPacket_<kV975> apply(const bp::GraphicsOverrideParameterPacket_<kV1001> &in)
    {
        // player_id dropped (lossy): a targeted override becomes a broadcast one.
        return {.keyframes = in.keyframes,
                .float_value = in.float_value,
                .vec3_value = in.vec3_value,
                .biome_id = in.biome_id,
                .parameter_id = in.parameter_id,
                .reset_parameter = in.reset_parameter};
    }
};

// --- 11 StartGame ----------------------------------------------------------------------
//
// One field inserted on the packet and two appended to LevelSettings. Both are wider than the
// field-count ladder, so their shared prefix is spelled out.

/** Every field LevelSettings has carried since 975. */
#define ENDWEAVE_LEVEL_SETTINGS_SHARED(out, in)                                             \
    (out).seed = (in).seed;                                                                 \
    (out).spawn_settings = (in).spawn_settings;                                             \
    (out).generator = (in).generator;                                                       \
    (out).game_type = (in).game_type;                                                       \
    (out).is_hardcore = (in).is_hardcore;                                                   \
    (out).game_difficulty = (in).game_difficulty;                                           \
    (out).default_spawn = (in).default_spawn;                                               \
    (out).achievements_disabled = (in).achievements_disabled;                               \
    (out).editor_world_type = (in).editor_world_type;                                       \
    (out).is_created_in_editor = (in).is_created_in_editor;                                 \
    (out).is_exported_from_editor = (in).is_exported_from_editor;                           \
    (out).time = (in).time;                                                                 \
    (out).education_edition_offer = (in).education_edition_offer;                           \
    (out).education_features_enabled = (in).education_features_enabled;                     \
    (out).education_product_id = (in).education_product_id;                                 \
    (out).rain_level = (in).rain_level;                                                     \
    (out).lightning_level = (in).lightning_level;                                           \
    (out).confirmed_platform_locked_content = (in).confirmed_platform_locked_content;       \
    (out).multiplayer_game_intent = (in).multiplayer_game_intent;                           \
    (out).lan_broadcast_intent = (in).lan_broadcast_intent;                                 \
    (out).xbl_broadcast_intent = (in).xbl_broadcast_intent;                                 \
    (out).platform_broadcast_intent = (in).platform_broadcast_intent;                       \
    (out).commands_enabled = (in).commands_enabled;                                         \
    (out).texture_packs_required = (in).texture_packs_required;                             \
    (out).game_rules = (in).game_rules;                                                     \
    (out).experiments = (in).experiments;                                                   \
    (out).experiments_previously_toggled = (in).experiments_previously_toggled;             \
    (out).bonus_chest_enabled = (in).bonus_chest_enabled;                                   \
    (out).start_with_map_enabled = (in).start_with_map_enabled;                             \
    (out).default_permissions = (in).default_permissions;                                   \
    (out).server_chunk_tick_range = (in).server_chunk_tick_range;                           \
    (out).has_locked_behavior_pack = (in).has_locked_behavior_pack;                         \
    (out).has_locked_resource_pack = (in).has_locked_resource_pack;                         \
    (out).is_from_locked_template = (in).is_from_locked_template;                           \
    (out).use_msa_gamertags_only = (in).use_msa_gamertags_only;                             \
    (out).is_from_world_template = (in).is_from_world_template;                             \
    (out).is_world_template_option_locked = (in).is_world_template_option_locked;           \
    (out).spawn_v1_villagers = (in).spawn_v1_villagers;                                     \
    (out).persona_disabled = (in).persona_disabled;                                         \
    (out).custom_skins_disabled = (in).custom_skins_disabled;                               \
    (out).emote_chat_muted = (in).emote_chat_muted;                                         \
    (out).base_game_version = (in).base_game_version;                                       \
    (out).limited_world_width = (in).limited_world_width;                                   \
    (out).limited_world_depth = (in).limited_world_depth;                                   \
    (out).nether_type = (in).nether_type;                                                   \
    (out).edu_shared_uri_resource = (in).edu_shared_uri_resource;                           \
    (out).override_force_experimental_gameplay = (in).override_force_experimental_gameplay; \
    (out).chat_restriction_level = (in).chat_restriction_level;                             \
    (out).disable_player_interactions = (in).disable_player_interactions

template <>
struct Conversion<bp::LevelSettings_<kV1001>, bp::LevelSettings_<kV975>> {
    static bp::LevelSettings_<kV1001> apply(const bp::LevelSettings_<kV975> &in)
    {
        bp::LevelSettings_<kV1001> out;
        ENDWEAVE_LEVEL_SETTINGS_SHARED(out, in);
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
        bp::LevelSettings_<kV975> out;
        ENDWEAVE_LEVEL_SETTINGS_SHARED(out, in);
        // server_editor_connection_policy, allow_anonymous_block_drops_in_editor_worlds dropped
        return out;
    }
};

/** Every StartGamePacket field that is not itself versioned. */
#define ENDWEAVE_START_GAME_SHARED(out, in)                                                   \
    (out).entity_id = (in).entity_id;                                                         \
    (out).runtime_id = (in).runtime_id;                                                       \
    (out).entity_game_type = (in).entity_game_type;                                           \
    (out).pos = (in).pos;                                                                     \
    (out).rot = (in).rot;                                                                     \
    (out).level_id = (in).level_id;                                                           \
    (out).level_name = (in).level_name;                                                       \
    (out).template_content_identity = (in).template_content_identity;                         \
    (out).is_trial = (in).is_trial;                                                           \
    (out).movement_settings = (in).movement_settings;                                         \
    (out).level_current_time = (in).level_current_time;                                       \
    (out).enchantment_seed = (in).enchantment_seed;                                           \
    (out).block_properties = (in).block_properties;                                           \
    (out).multiplayer_correlation_id = (in).multiplayer_correlation_id;                       \
    (out).enable_item_stack_net_manager = (in).enable_item_stack_net_manager;                 \
    (out).server_version = (in).server_version;                                               \
    (out).player_property_data = (in).player_property_data;                                   \
    (out).server_block_type_registry_checksum = (in).server_block_type_registry_checksum;     \
    (out).world_template_id = (in).world_template_id;                                         \
    (out).server_enabled_client_side_generation = (in).server_enabled_client_side_generation; \
    (out).block_network_ids_are_hashes = (in).block_network_ids_are_hashes;                   \
    (out).network_permissions = (in).network_permissions;                                     \
    (out).server_telemetry_data = (in).server_telemetry_data

template <>
struct Conversion<bp::StartGamePacket_<kV1001>, bp::StartGamePacket_<kV975>> {
    static bp::StartGamePacket_<kV1001> apply(const bp::StartGamePacket_<kV975> &in)
    {
        bp::StartGamePacket_<kV1001> out;
        ENDWEAVE_START_GAME_SHARED(out, in);
        out.settings = convert<bp::LevelSettings_<kV1001>>(in.settings);
        out.is_chat_logging = false; // polyfill: a 975 server never asks the client to log chat
        out.server_configuration_join_info =
            convert<std::optional<bp::ServerConfigurationJoinInfo_<kV1001>>>(in.server_configuration_join_info);
        return out;
    }
};

template <>
struct Conversion<bp::StartGamePacket_<kV975>, bp::StartGamePacket_<kV1001>> {
    static bp::StartGamePacket_<kV975> apply(const bp::StartGamePacket_<kV1001> &in)
    {
        bp::StartGamePacket_<kV975> out;
        ENDWEAVE_START_GAME_SHARED(out, in);
        out.settings = convert<bp::LevelSettings_<kV975>>(in.settings);
        // is_chat_logging dropped (lossy)
        out.server_configuration_join_info =
            convert<std::optional<bp::ServerConfigurationJoinInfo_<kV975>>>(in.server_configuration_join_info);
        return out;
    }
};

// --- 74 BossEvent ----------------------------------------------------------------------
//
// 984 cerealised the packet: player_id moved ahead of event_type, darken_screen went, and the
// eight switch arms flattened so every field is now written unconditionally.

template <>
struct Conversion<bp::BossEventPacket_<kV1001>, bp::BossEventPacket_<kV975>> {
    static bp::BossEventPacket_<kV1001> apply(const bp::BossEventPacket_<kV975> &in)
    {
        // The 975 form only wrote the fields its event type selected and left the rest
        // default-constructed; 1001 writes them all, so the unselected ones go out as defaults.
        // darken_screen has nowhere to go (lossy).
        return {.boss_id = in.boss_id,
                .player_id = in.player_id,
                .event_type = in.event_type,
                .name = in.name,
                .filtered_name = in.filtered_name,
                .health_percent = in.health_percent,
                .color = in.color,
                .overlay = in.overlay};
    }
};

template <>
struct Conversion<bp::BossEventPacket_<kV975>, bp::BossEventPacket_<kV1001>> {
    static bp::BossEventPacket_<kV975> apply(const bp::BossEventPacket_<kV1001> &in)
    {
        return {.boss_id = in.boss_id,
                .event_type = in.event_type,
                .player_id = in.player_id,
                .name = in.name,
                .filtered_name = in.filtered_name,
                .health_percent = in.health_percent,
                .darken_screen = 0, // polyfill: 1001 carries no value to restore
                .color = in.color,
                .overlay = in.overlay};
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

// PacketHandlers::map takes a function pointer, so each packet needs a named entry point rather
// than a bare convert<To> instantiation.
#define ENDWEAVE_PACKET(name, type)                                   \
    bp::type##_<kV1001> upgrade##name(const bp::type##_<kV975> &in)   \
    {                                                                 \
        return convert<bp::type##_<kV1001>>(in);                      \
    }                                                                 \
    bp::type##_<kV975> downgrade##name(const bp::type##_<kV1001> &in) \
    {                                                                 \
        return convert<bp::type##_<kV975>>(in);                       \
    }

ENDWEAVE_PACKET(AttributeLayerSync, ClientboundAttributeLayerSyncPacket)
ENDWEAVE_PACKET(Diagnostics, ServerboundDiagnosticsPacket)
ENDWEAVE_PACKET(SubChunkRequest, SubChunkRequestPacket)
ENDWEAVE_PACKET(PresenceInfo, ServerPresenceInfoPacket)
ENDWEAVE_PACKET(BlobStatus, ClientCacheBlobStatusPacket)
ENDWEAVE_PACKET(GraphicsOverride, GraphicsOverrideParameterPacket)
ENDWEAVE_PACKET(StartGame, StartGamePacket)
ENDWEAVE_PACKET(BossEvent, BossEventPacket)
ENDWEAVE_PACKET(BiomeDefinitionList, BiomeDefinitionListPacket)
ENDWEAVE_PACKET(PrimitiveShapes, PrimitiveShapesPacket)
ENDWEAVE_PACKET(MobArmorEquipment, MobArmorEquipmentPacket)
ENDWEAVE_PACKET(InventoryContent, InventoryContentPacket)
ENDWEAVE_PACKET(InventoryTransaction, InventoryTransactionPacket)

#undef ENDWEAVE_PACKET

} // namespace

void Protocol<ProtocolVersion::V1001>::registerPackets()
{
    using Ids = MinecraftPacketIds;

// Both directions of one packet, named by the source-version type so the overload is pinned.
#define ENDWEAVE_REGISTER(id, name, type)                                              \
    registerUpgrade(Ids::id, PacketHandlers::map<bp::type##_<kV975>>(&upgrade##name)); \
    registerDowngrade(Ids::id, PacketHandlers::map<bp::type##_<kV1001>>(&downgrade##name))

    ENDWEAVE_REGISTER(StartGame, StartGame, StartGamePacket);
    ENDWEAVE_REGISTER(InventoryTransaction, InventoryTransaction, InventoryTransactionPacket);
    ENDWEAVE_REGISTER(MobArmorEquipment, MobArmorEquipment, MobArmorEquipmentPacket);
    ENDWEAVE_REGISTER(InventoryContent, InventoryContent, InventoryContentPacket);
    ENDWEAVE_REGISTER(BossEvent, BossEvent, BossEventPacket);
    ENDWEAVE_REGISTER(BiomeDefinitionList, BiomeDefinitionList, BiomeDefinitionListPacket);
    ENDWEAVE_REGISTER(ClientCacheBlobStatus, BlobStatus, ClientCacheBlobStatusPacket);
    ENDWEAVE_REGISTER(SubChunkRequest, SubChunkRequest, SubChunkRequestPacket);
    ENDWEAVE_REGISTER(ServerboundDiagnostics, Diagnostics, ServerboundDiagnosticsPacket);
    ENDWEAVE_REGISTER(ServerScriptDebugDrawer, PrimitiveShapes, PrimitiveShapesPacket);
    ENDWEAVE_REGISTER(GraphicsParameterOverride, GraphicsOverride, GraphicsOverrideParameterPacket);
    ENDWEAVE_REGISTER(ClientboundAttributeLayerSync, AttributeLayerSync, ClientboundAttributeLayerSyncPacket);
    ENDWEAVE_REGISTER(ServerPresenceInfo, PresenceInfo, ServerPresenceInfoPacket);

#undef ENDWEAVE_REGISTER

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
