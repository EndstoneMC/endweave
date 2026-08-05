#include "endweave/protocols/v1001/game.h"

#include "endweave/protocols/v1001/presence.h"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::ExperimentToggle Transformer<bp::ExperimentData, bp::ExperimentToggle>::transform(bp::ExperimentData &&from)
{
    bp::ExperimentToggle to;
    to.name = std::move(from.name);
    to.enabled = from.enabled;
    return to;
}

bp::GameRule Transformer<bp::legacy::GameRule_<1001>, bp::GameRule>::transform(bp::legacy::GameRule_<1001> &&from)
{
    bp::GameRule to;
    to.name = std::move(from.name);
    to.can_be_modified_by_player = from.can_be_modified_by_player;
    // ENDWEAVE: 2168 holds the integer alternative signed. The same 32 bits reach the wire.
    std::visit(
        [&to](auto &&alt) {
            using Alt = std::decay_t<decltype(alt)>;
            if constexpr (std::is_same_v<Alt, std::uint32_t>) {
                to.value = static_cast<std::int32_t>(alt);
            }
            else {
                to.value = alt;
            }
        },
        from.value);
    return to;
}

bp::LevelSettings_<2168> Transformer<bp::LevelSettings_<1001>, bp::LevelSettings_<2168>>::transform(
    bp::LevelSettings_<1001> &&from)
{
    bp::LevelSettings_<2168> to;
    // ENDWEAVE: 2168 reads the seed unsigned; the same 64 bits are the same world.
    to.seed = static_cast<std::uint64_t>(from.seed);
    to.spawn_settings = std::move(from.spawn_settings);
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
    to.education_product_id = std::move(from.education_product_id);
    to.rain_level = from.rain_level;
    to.lightning_level = from.lightning_level;
    to.confirmed_platform_locked_content = from.confirmed_platform_locked_content;
    to.multiplayer_game_intent = from.multiplayer_game_intent;
    to.lan_broadcast_intent = from.lan_broadcast_intent;
    to.xbl_broadcast_intent = from.xbl_broadcast_intent;
    to.platform_broadcast_intent = from.platform_broadcast_intent;
    to.commands_enabled = from.commands_enabled;
    to.texture_packs_required = from.texture_packs_required;
    // ENDWEAVE: 2168 only nests these — the rules under rule_data, the toggles and the ever-toggled flag
    // under experiments. Nothing about the values moved.
    to.rule_data.rules = ew::transform(std::move(from.game_rules));
    to.experiments.toggles = ew::transform(std::move(from.experiments));
    to.experiments.experiments_ever_toggled = from.experiments_previously_toggled;
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
    to.base_game_version = std::move(from.base_game_version);
    to.limited_world_width = from.limited_world_width;
    to.limited_world_depth = from.limited_world_depth;
    to.nether_type = from.nether_type;
    to.edu_shared_uri_resource = std::move(from.edu_shared_uri_resource);
    to.override_force_experimental_gameplay = from.override_force_experimental_gameplay;
    to.chat_restriction_level = from.chat_restriction_level;
    to.disable_player_interactions = from.disable_player_interactions;
    to.server_editor_connection_policy = from.server_editor_connection_policy;
    to.allow_anonymous_block_drops_in_editor_worlds = from.allow_anonymous_block_drops_in_editor_worlds;
    return to;
}

bp::ServerBlockProperty_<2168> Transformer<bp::BlockEntry, bp::ServerBlockProperty_<2168>>::transform(
    bp::BlockEntry &&from)
{
    bp::ServerBlockProperty_<2168> to;
    to.block_name = std::move(from.name);
    to.block_definition = std::move(from.properties);
    return to;
}

bp::StartGamePacket_<2168> Transformer<bp::StartGamePacket_<1001>, bp::StartGamePacket_<2168>>::transform(
    bp::StartGamePacket_<1001> &&from)
{
    bp::StartGamePacket_<2168> to;
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.entity_game_type = from.entity_game_type;
    to.pos = from.pos;
    to.rot = from.rot;
    to.settings = ew::transform(std::move(from.settings));
    to.level_id = std::move(from.level_id);
    to.level_name = std::move(from.level_name);
    to.template_content_identity = std::move(from.template_content_identity);
    to.is_trial = from.is_trial;
    to.movement_settings = from.movement_settings;
    // ENDWEAVE: 2168 reads the tick count unsigned; same bits, as with the seed.
    to.level_current_time = static_cast<std::uint64_t>(from.level_current_time);
    to.enchantment_seed = from.enchantment_seed;
    to.block_properties = ew::transform(std::move(from.block_properties));
    to.multiplayer_correlation_id = std::move(from.multiplayer_correlation_id);
    to.enable_item_stack_net_manager = from.enable_item_stack_net_manager;
    to.server_version = std::move(from.server_version);
    to.player_property_data = std::move(from.player_property_data);
    // ENDWEAVE: zeroed to skip validation -- the checksum covers the server's block registry
    // and a client on another version computes a different one, then drops the connection.
    to.server_block_type_registry_checksum = 0;
    to.world_template_id = from.world_template_id;
    to.server_enabled_client_side_generation = from.server_enabled_client_side_generation;
    to.block_network_ids_are_hashes = from.block_network_ids_are_hashes;
    to.network_permissions = from.network_permissions;
    // ENDWEAVE: is_chat_logging is dropped; 2168 no longer tells the client the server logs chat.
    to.server_configuration_join_info = ew::transform(std::move(from.server_configuration_join_info));
    to.server_telemetry_data = std::move(from.server_telemetry_data);
    return to;
}

} // namespace endweave
