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

inline constexpr auto kEnvironmentAttributeFields = fieldList(
    [](auto &v) -> decltype((v.name)) {
        return v.name;
    },
    [](auto &v) -> decltype((v.from_attribute)) {
        return v.from_attribute;
    },
    [](auto &v) -> decltype((v.attribute)) {
        return v.attribute;
    },
    [](auto &v) -> decltype((v.to_attribute)) {
        return v.to_attribute;
    },
    [](auto &v) -> decltype((v.current_transition_ticks)) {
        return v.current_transition_ticks;
    },
    [](auto &v) -> decltype((v.total_transition_ticks)) {
        return v.total_transition_ticks;
    },
    [](auto &v) -> decltype((v.easing)) {
        return v.easing;
    },
    [](auto &v) -> decltype((v.local_transition_ticks)) {
        return v.local_transition_ticks;
    },
    [](auto &v) -> decltype((v.noise_transition)) {
        return v.noise_transition;
    });

/** EnvironmentAttributeData appended local_transition_ticks and noise_transition. */
template <>
struct Conversion<bp::EnvironmentAttributeData_<kV1001>, bp::EnvironmentAttributeData_<kV975>> {
    static bp::EnvironmentAttributeData_<kV1001> apply(const bp::EnvironmentAttributeData_<kV975> &in)
    {
        auto out = kEnvironmentAttributeFields.copy<bp::EnvironmentAttributeData_<kV1001>>(in);
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
        return kEnvironmentAttributeFields.copy<bp::EnvironmentAttributeData_<kV975>>(in);
    }
};

inline constexpr auto kAttributeLayerFields = fieldList(
    [](auto &v) -> decltype((v.name)) {
        return v.name;
    },
    [](auto &v) -> decltype((v.dimension_id)) {
        return v.dimension_id;
    },
    [](auto &v) -> decltype((v.settings)) {
        return v.settings;
    },
    [](auto &v) -> decltype((v.attributes)) {
        return v.attributes;
    },
    [](auto &v) -> decltype((v.noise_name)) {
        return v.noise_name;
    });

/** AttributeLayerData inserted noise_name after name. */
template <>
struct Conversion<bp::AttributeLayerData_<kV1001>, bp::AttributeLayerData_<kV975>> {
    static bp::AttributeLayerData_<kV1001> apply(const bp::AttributeLayerData_<kV975> &in)
    {
        auto out = kAttributeLayerFields.copy<bp::AttributeLayerData_<kV1001>>(in);
        out.noise_name = std::nullopt; // polyfill
        return out;
    }
};

template <>
struct Conversion<bp::AttributeLayerData_<kV975>, bp::AttributeLayerData_<kV1001>> {
    static bp::AttributeLayerData_<kV975> apply(const bp::AttributeLayerData_<kV1001> &in)
    {
        // noise_name dropped (lossy)
        return kAttributeLayerFields.copy<bp::AttributeLayerData_<kV975>>(in);
    }
};

// --- 315 ServerboundDiagnostics --------------------------------------------------------

inline constexpr auto kDiagnosticsFields = fieldList(
    [](auto &v) -> decltype((v.avg_fps)) {
        return v.avg_fps;
    },
    [](auto &v) -> decltype((v.avg_server_sim_tick_time_ms)) {
        return v.avg_server_sim_tick_time_ms;
    },
    [](auto &v) -> decltype((v.avg_client_sim_tick_time_ms)) {
        return v.avg_client_sim_tick_time_ms;
    },
    [](auto &v) -> decltype((v.avg_begin_frame_time_ms)) {
        return v.avg_begin_frame_time_ms;
    },
    [](auto &v) -> decltype((v.avg_input_time_ms)) {
        return v.avg_input_time_ms;
    },
    [](auto &v) -> decltype((v.avg_render_time_ms)) {
        return v.avg_render_time_ms;
    },
    [](auto &v) -> decltype((v.avg_end_frame_time_ms)) {
        return v.avg_end_frame_time_ms;
    },
    [](auto &v) -> decltype((v.avg_remainder_time_percent)) {
        return v.avg_remainder_time_percent;
    },
    [](auto &v) -> decltype((v.avg_unaccounted_time_percent)) {
        return v.avg_unaccounted_time_percent;
    },
    [](auto &v) -> decltype((v.memory_category_values)) {
        return v.memory_category_values;
    },
    [](auto &v) -> decltype((v.entity_diagnostics)) {
        return v.entity_diagnostics;
    },
    [](auto &v) -> decltype((v.system_diagnostics)) {
        return v.system_diagnostics;
    },
    [](auto &v) -> decltype((v.whisker_scopes)) {
        return v.whisker_scopes;
    });

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
        return kDiagnosticsFields.copy<bp::ServerboundDiagnosticsPacket_<kV1001>>(in);
    }
};

template <>
struct Conversion<bp::ServerboundDiagnosticsPacket_<kV975>, bp::ServerboundDiagnosticsPacket_<kV1001>> {
    static bp::ServerboundDiagnosticsPacket_<kV975> apply(const bp::ServerboundDiagnosticsPacket_<kV1001> &in)
    {
        return kDiagnosticsFields.copy<bp::ServerboundDiagnosticsPacket_<kV975>>(in);
    }
};

// --- 175 SubChunkRequest ---------------------------------------------------------------
//
// The 979 cerealisation reorders the packet and reworks how two fields encode, but carries the
// same three values, so neither direction loses anything. SubChunkPos keeps its field list --
// only the encoding moved, varint32 to fixed int32 -- so it needs no converter.

inline constexpr auto kSubChunkRequestFields = fieldList(
    [](auto &v) -> decltype((v.dimension_type)) {
        return v.dimension_type;
    },
    [](auto &v) -> decltype((v.center_pos)) {
        return v.center_pos;
    },
    [](auto &v) -> decltype((v.sub_chunk_pos_offsets)) {
        return v.sub_chunk_pos_offsets;
    });

/** 979 moved center_pos behind the offsets; matching by name makes the reorder a non-event. */
template <>
struct Conversion<bp::SubChunkRequestPacket_<kV1001>, bp::SubChunkRequestPacket_<kV975>> {
    static bp::SubChunkRequestPacket_<kV1001> apply(const bp::SubChunkRequestPacket_<kV975> &in)
    {
        return kSubChunkRequestFields.copy<bp::SubChunkRequestPacket_<kV1001>>(in);
    }
};

template <>
struct Conversion<bp::SubChunkRequestPacket_<kV975>, bp::SubChunkRequestPacket_<kV1001>> {
    static bp::SubChunkRequestPacket_<kV975> apply(const bp::SubChunkRequestPacket_<kV1001> &in)
    {
        return kSubChunkRequestFields.copy<bp::SubChunkRequestPacket_<kV975>>(in);
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

inline constexpr auto kBlobStatusFields = fieldList(
    [](auto &v) -> decltype((v.missing_count)) {
        return v.missing_count;
    },
    [](auto &v) -> decltype((v.found_count)) {
        return v.found_count;
    },
    [](auto &v) -> decltype((v.missing_ids)) {
        return v.missing_ids;
    },
    [](auto &v) -> decltype((v.found_ids)) {
        return v.found_ids;
    });

template <>
struct Conversion<bp::ClientCacheBlobStatusPacket_<kV1001>, bp::ClientCacheBlobStatusPacket_<kV975>> {
    static bp::ClientCacheBlobStatusPacket_<kV1001> apply(const bp::ClientCacheBlobStatusPacket_<kV975> &in)
    {
        // 1001 prefixes each list instead of carrying the counts up front, so they just go.
        return kBlobStatusFields.copy<bp::ClientCacheBlobStatusPacket_<kV1001>>(in);
    }
};

template <>
struct Conversion<bp::ClientCacheBlobStatusPacket_<kV975>, bp::ClientCacheBlobStatusPacket_<kV1001>> {
    static bp::ClientCacheBlobStatusPacket_<kV975> apply(const bp::ClientCacheBlobStatusPacket_<kV1001> &in)
    {
        auto out = kBlobStatusFields.copy<bp::ClientCacheBlobStatusPacket_<kV975>>(in);
        // 975 states each length up front; 1001 only implies it through the list prefix.
        out.missing_count = static_cast<std::uint32_t>(out.missing_ids.size());
        out.found_count = static_cast<std::uint32_t>(out.found_ids.size());
        return out;
    }
};

// --- 331 GraphicsOverrideParameter -----------------------------------------------------

inline constexpr auto kGraphicsOverrideFields = fieldList(
    [](auto &v) -> decltype((v.keyframes)) {
        return v.keyframes;
    },
    [](auto &v) -> decltype((v.float_value)) {
        return v.float_value;
    },
    [](auto &v) -> decltype((v.vec3_value)) {
        return v.vec3_value;
    },
    [](auto &v) -> decltype((v.biome_id)) {
        return v.biome_id;
    },
    [](auto &v) -> decltype((v.parameter_id)) {
        return v.parameter_id;
    },
    [](auto &v) -> decltype((v.reset_parameter)) {
        return v.reset_parameter;
    },
    [](auto &v) -> decltype((v.player_id)) {
        return v.player_id;
    });

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
        return kGraphicsOverrideFields.copy<bp::GraphicsOverrideParameterPacket_<kV1001>>(in);
    }
};

template <>
struct Conversion<bp::GraphicsOverrideParameterPacket_<kV975>, bp::GraphicsOverrideParameterPacket_<kV1001>> {
    static bp::GraphicsOverrideParameterPacket_<kV975> apply(const bp::GraphicsOverrideParameterPacket_<kV1001> &in)
    {
        return kGraphicsOverrideFields.copy<bp::GraphicsOverrideParameterPacket_<kV975>>(in);
    }
};

// --- 11 StartGame ----------------------------------------------------------------------
//
// One field inserted on the packet and two appended to LevelSettings. Both are wider than the
// field-count ladder, so their shared prefix is spelled out.

inline constexpr auto kLevelSettingsFields = fieldList(
    [](auto &v) -> decltype((v.seed)) {
        return v.seed;
    },
    [](auto &v) -> decltype((v.spawn_settings)) {
        return v.spawn_settings;
    },
    [](auto &v) -> decltype((v.generator)) {
        return v.generator;
    },
    [](auto &v) -> decltype((v.game_type)) {
        return v.game_type;
    },
    [](auto &v) -> decltype((v.is_hardcore)) {
        return v.is_hardcore;
    },
    [](auto &v) -> decltype((v.game_difficulty)) {
        return v.game_difficulty;
    },
    [](auto &v) -> decltype((v.default_spawn)) {
        return v.default_spawn;
    },
    [](auto &v) -> decltype((v.achievements_disabled)) {
        return v.achievements_disabled;
    },
    [](auto &v) -> decltype((v.editor_world_type)) {
        return v.editor_world_type;
    },
    [](auto &v) -> decltype((v.is_created_in_editor)) {
        return v.is_created_in_editor;
    },
    [](auto &v) -> decltype((v.is_exported_from_editor)) {
        return v.is_exported_from_editor;
    },
    [](auto &v) -> decltype((v.time)) {
        return v.time;
    },
    [](auto &v) -> decltype((v.education_edition_offer)) {
        return v.education_edition_offer;
    },
    [](auto &v) -> decltype((v.education_features_enabled)) {
        return v.education_features_enabled;
    },
    [](auto &v) -> decltype((v.education_product_id)) {
        return v.education_product_id;
    },
    [](auto &v) -> decltype((v.rain_level)) {
        return v.rain_level;
    },
    [](auto &v) -> decltype((v.lightning_level)) {
        return v.lightning_level;
    },
    [](auto &v) -> decltype((v.confirmed_platform_locked_content)) {
        return v.confirmed_platform_locked_content;
    },
    [](auto &v) -> decltype((v.multiplayer_game_intent)) {
        return v.multiplayer_game_intent;
    },
    [](auto &v) -> decltype((v.lan_broadcast_intent)) {
        return v.lan_broadcast_intent;
    },
    [](auto &v) -> decltype((v.xbl_broadcast_intent)) {
        return v.xbl_broadcast_intent;
    },
    [](auto &v) -> decltype((v.platform_broadcast_intent)) {
        return v.platform_broadcast_intent;
    },
    [](auto &v) -> decltype((v.commands_enabled)) {
        return v.commands_enabled;
    },
    [](auto &v) -> decltype((v.texture_packs_required)) {
        return v.texture_packs_required;
    },
    [](auto &v) -> decltype((v.game_rules)) {
        return v.game_rules;
    },
    [](auto &v) -> decltype((v.experiments)) {
        return v.experiments;
    },
    [](auto &v) -> decltype((v.experiments_previously_toggled)) {
        return v.experiments_previously_toggled;
    },
    [](auto &v) -> decltype((v.bonus_chest_enabled)) {
        return v.bonus_chest_enabled;
    },
    [](auto &v) -> decltype((v.start_with_map_enabled)) {
        return v.start_with_map_enabled;
    },
    [](auto &v) -> decltype((v.default_permissions)) {
        return v.default_permissions;
    },
    [](auto &v) -> decltype((v.server_chunk_tick_range)) {
        return v.server_chunk_tick_range;
    },
    [](auto &v) -> decltype((v.has_locked_behavior_pack)) {
        return v.has_locked_behavior_pack;
    },
    [](auto &v) -> decltype((v.has_locked_resource_pack)) {
        return v.has_locked_resource_pack;
    },
    [](auto &v) -> decltype((v.is_from_locked_template)) {
        return v.is_from_locked_template;
    },
    [](auto &v) -> decltype((v.use_msa_gamertags_only)) {
        return v.use_msa_gamertags_only;
    },
    [](auto &v) -> decltype((v.is_from_world_template)) {
        return v.is_from_world_template;
    },
    [](auto &v) -> decltype((v.is_world_template_option_locked)) {
        return v.is_world_template_option_locked;
    },
    [](auto &v) -> decltype((v.spawn_v1_villagers)) {
        return v.spawn_v1_villagers;
    },
    [](auto &v) -> decltype((v.persona_disabled)) {
        return v.persona_disabled;
    },
    [](auto &v) -> decltype((v.custom_skins_disabled)) {
        return v.custom_skins_disabled;
    },
    [](auto &v) -> decltype((v.emote_chat_muted)) {
        return v.emote_chat_muted;
    },
    [](auto &v) -> decltype((v.base_game_version)) {
        return v.base_game_version;
    },
    [](auto &v) -> decltype((v.limited_world_width)) {
        return v.limited_world_width;
    },
    [](auto &v) -> decltype((v.limited_world_depth)) {
        return v.limited_world_depth;
    },
    [](auto &v) -> decltype((v.nether_type)) {
        return v.nether_type;
    },
    [](auto &v) -> decltype((v.edu_shared_uri_resource)) {
        return v.edu_shared_uri_resource;
    },
    [](auto &v) -> decltype((v.override_force_experimental_gameplay)) {
        return v.override_force_experimental_gameplay;
    },
    [](auto &v) -> decltype((v.chat_restriction_level)) {
        return v.chat_restriction_level;
    },
    [](auto &v) -> decltype((v.disable_player_interactions)) {
        return v.disable_player_interactions;
    },
    [](auto &v) -> decltype((v.server_editor_connection_policy)) {
        return v.server_editor_connection_policy;
    },
    [](auto &v) -> decltype((v.allow_anonymous_block_drops_in_editor_worlds)) {
        return v.allow_anonymous_block_drops_in_editor_worlds;
    });

inline constexpr auto kStartGameFields = fieldList(
    [](auto &v) -> decltype((v.entity_id)) {
        return v.entity_id;
    },
    [](auto &v) -> decltype((v.runtime_id)) {
        return v.runtime_id;
    },
    [](auto &v) -> decltype((v.entity_game_type)) {
        return v.entity_game_type;
    },
    [](auto &v) -> decltype((v.pos)) {
        return v.pos;
    },
    [](auto &v) -> decltype((v.rot)) {
        return v.rot;
    },
    [](auto &v) -> decltype((v.settings)) {
        return v.settings;
    },
    [](auto &v) -> decltype((v.level_id)) {
        return v.level_id;
    },
    [](auto &v) -> decltype((v.level_name)) {
        return v.level_name;
    },
    [](auto &v) -> decltype((v.template_content_identity)) {
        return v.template_content_identity;
    },
    [](auto &v) -> decltype((v.is_trial)) {
        return v.is_trial;
    },
    [](auto &v) -> decltype((v.movement_settings)) {
        return v.movement_settings;
    },
    [](auto &v) -> decltype((v.level_current_time)) {
        return v.level_current_time;
    },
    [](auto &v) -> decltype((v.enchantment_seed)) {
        return v.enchantment_seed;
    },
    [](auto &v) -> decltype((v.block_properties)) {
        return v.block_properties;
    },
    [](auto &v) -> decltype((v.multiplayer_correlation_id)) {
        return v.multiplayer_correlation_id;
    },
    [](auto &v) -> decltype((v.enable_item_stack_net_manager)) {
        return v.enable_item_stack_net_manager;
    },
    [](auto &v) -> decltype((v.server_version)) {
        return v.server_version;
    },
    [](auto &v) -> decltype((v.player_property_data)) {
        return v.player_property_data;
    },
    [](auto &v) -> decltype((v.server_block_type_registry_checksum)) {
        return v.server_block_type_registry_checksum;
    },
    [](auto &v) -> decltype((v.world_template_id)) {
        return v.world_template_id;
    },
    [](auto &v) -> decltype((v.server_enabled_client_side_generation)) {
        return v.server_enabled_client_side_generation;
    },
    [](auto &v) -> decltype((v.block_network_ids_are_hashes)) {
        return v.block_network_ids_are_hashes;
    },
    [](auto &v) -> decltype((v.network_permissions)) {
        return v.network_permissions;
    },
    [](auto &v) -> decltype((v.server_configuration_join_info)) {
        return v.server_configuration_join_info;
    },
    [](auto &v) -> decltype((v.server_telemetry_data)) {
        return v.server_telemetry_data;
    },
    [](auto &v) -> decltype((v.is_chat_logging)) {
        return v.is_chat_logging;
    });

/** LevelSettings appended two editor fields at 1001; StartGame inserted is_chat_logging. */
template <>
struct Conversion<bp::LevelSettings_<kV1001>, bp::LevelSettings_<kV975>> {
    static bp::LevelSettings_<kV1001> apply(const bp::LevelSettings_<kV975> &in)
    {
        auto out = kLevelSettingsFields.copy<bp::LevelSettings_<kV1001>>(in);
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
        return kLevelSettingsFields.copy<bp::LevelSettings_<kV975>>(in);
    }
};

template <>
struct Conversion<bp::StartGamePacket_<kV1001>, bp::StartGamePacket_<kV975>> {
    static bp::StartGamePacket_<kV1001> apply(const bp::StartGamePacket_<kV975> &in)
    {
        // settings and server_configuration_join_info are versioned; the copy recurses.
        auto out = kStartGameFields.copy<bp::StartGamePacket_<kV1001>>(in);
        out.is_chat_logging = false; // polyfill: a 975 server never asks the client to log chat
        return out;
    }
};

template <>
struct Conversion<bp::StartGamePacket_<kV975>, bp::StartGamePacket_<kV1001>> {
    static bp::StartGamePacket_<kV975> apply(const bp::StartGamePacket_<kV1001> &in)
    {
        // is_chat_logging dropped (lossy)
        return kStartGameFields.copy<bp::StartGamePacket_<kV975>>(in);
    }
};

// --- 74 BossEvent ----------------------------------------------------------------------
//
// 984 cerealised the packet: player_id moved ahead of event_type, darken_screen went, and the
// eight switch arms flattened so every field is now written unconditionally.

inline constexpr auto kBossEventFields = fieldList(
    [](auto &v) -> decltype((v.boss_id)) {
        return v.boss_id;
    },
    [](auto &v) -> decltype((v.event_type)) {
        return v.event_type;
    },
    [](auto &v) -> decltype((v.player_id)) {
        return v.player_id;
    },
    [](auto &v) -> decltype((v.name)) {
        return v.name;
    },
    [](auto &v) -> decltype((v.filtered_name)) {
        return v.filtered_name;
    },
    [](auto &v) -> decltype((v.health_percent)) {
        return v.health_percent;
    },
    [](auto &v) -> decltype((v.darken_screen)) {
        return v.darken_screen;
    },
    [](auto &v) -> decltype((v.color)) {
        return v.color;
    },
    [](auto &v) -> decltype((v.overlay)) {
        return v.overlay;
    });

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
        return kBossEventFields.copy<bp::BossEventPacket_<kV1001>>(in);
    }
};

template <>
struct Conversion<bp::BossEventPacket_<kV975>, bp::BossEventPacket_<kV1001>> {
    static bp::BossEventPacket_<kV975> apply(const bp::BossEventPacket_<kV1001> &in)
    {
        auto out = kBossEventFields.copy<bp::BossEventPacket_<kV975>>(in);
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
