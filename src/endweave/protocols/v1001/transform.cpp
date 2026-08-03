#include "endweave/protocols/v1001/transform.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::v2168::SerializedNetworkItemStackDescriptor Transformer<bp::v1001::SerializedNetworkItemStackDescriptor>::upgrade(
    bp::v1001::SerializedNetworkItemStackDescriptor &&from)
{
    bp::v2168::SerializedNetworkItemStackDescriptor to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    if (from.net_id_variant.has_value()) {
        const auto &net_id = from.net_id_variant.value();
        if (const auto *request_id = std::get_if<bp::ItemStackRequestId>(&net_id)) {
            to.net_id_variant = -2 * request_id->id - 1;
        }
        else if (const auto *legacy_id = std::get_if<bp::ItemStackLegacyRequestId>(&net_id)) {
            to.net_id_variant = -2 * legacy_id->id;
        }
        else {
            to.net_id_variant = std::get<bp::ItemStackNetId>(net_id).id;
        }
    }
    to.block_runtime_id = from.block_runtime_id;
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

bp::v2168::InventoryContentPacket Transformer<bp::v1001::InventoryContentPacket>::upgrade(
    bp::v1001::InventoryContentPacket &&from)
{
    bp::v2168::InventoryContentPacket to;
    to.inventory_id = from.inventory_id;
    to.slots = ew::upgrade(from.slots);
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::upgrade(from.storage_item);
    return to;
}

bp::ExperimentToggle Transformer<bp::ExperimentData>::upgrade(bp::ExperimentData &&from)
{
    bp::ExperimentToggle to;
    to.name = std::move(from.name);
    to.enabled = from.enabled;
    return to;
}

bp::v2168::LevelSettings Transformer<bp::v1001::LevelSettings>::upgrade(bp::v1001::LevelSettings &&from)
{
    bp::v2168::LevelSettings to;
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
    to.rule_data.rules = std::move(from.game_rules);
    to.experiments.toggles = ew::upgrade(from.experiments);
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

bp::v2168::ServerBlockProperty Transformer<bp::BlockEntry>::upgrade(bp::BlockEntry &&from)
{
    bp::v2168::ServerBlockProperty to;
    to.block_name = std::move(from.name);
    to.block_definition = std::move(from.properties);
    return to;
}

bp::v2168::PresenceConfiguration Transformer<bp::v1001::PresenceConfiguration>::upgrade(
    bp::v1001::PresenceConfiguration &&from)
{
    bp::v2168::PresenceConfiguration to;
    to.rich_presence_id = std::move(from.rich_presence_id);
    return to;
}

bp::v2168::GatheringsConfigurationJoinInfo Transformer<bp::v1001::GatheringsConfigurationJoinInfo>::upgrade(
    bp::v1001::GatheringsConfigurationJoinInfo &&from)
{
    bp::v2168::GatheringsConfigurationJoinInfo to;
    to.experience_id = from.experience_id;
    to.experience_name = std::move(from.experience_name);
    to.experience_world_id = from.experience_world_id;
    to.experience_world_name = std::move(from.experience_world_name);
    to.creator_id = std::move(from.creator_id);
    to.target_id = from.target_id;
    to.scenario_id = std::move(from.scenario_id);
    to.server_id = std::move(from.server_id);
    return to;
}

bp::v2168::ServerConfigurationJoinInfo Transformer<bp::v1001::ServerConfigurationJoinInfo>::upgrade(
    bp::v1001::ServerConfigurationJoinInfo &&from)
{
    bp::v2168::ServerConfigurationJoinInfo to;
    to.gatherings_configuration_join_info = ew::upgrade(from.gatherings_configuration_join_info);
    to.client_store_entrypoint_configuration = std::move(from.client_store_entrypoint_configuration);
    to.presence_configuration = ew::upgrade(from.presence_configuration);
    return to;
}

bp::v2168::StartGamePacket Transformer<bp::v1001::StartGamePacket>::upgrade(bp::v1001::StartGamePacket &&from)
{
    bp::v2168::StartGamePacket to;
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.entity_game_type = from.entity_game_type;
    to.pos = from.pos;
    to.rot = from.rot;
    to.settings = ew::upgrade(from.settings);
    to.level_id = std::move(from.level_id);
    to.level_name = std::move(from.level_name);
    to.template_content_identity = std::move(from.template_content_identity);
    to.is_trial = from.is_trial;
    to.movement_settings = from.movement_settings;
    to.level_current_time = static_cast<std::uint64_t>(from.level_current_time);
    to.enchantment_seed = from.enchantment_seed;
    to.block_properties = ew::upgrade(from.block_properties);
    to.multiplayer_correlation_id = std::move(from.multiplayer_correlation_id);
    to.enable_item_stack_net_manager = from.enable_item_stack_net_manager;
    to.server_version = std::move(from.server_version);
    to.player_property_data = std::move(from.player_property_data);
    to.server_block_type_registry_checksum = from.server_block_type_registry_checksum;
    to.world_template_id = from.world_template_id;
    to.server_enabled_client_side_generation = from.server_enabled_client_side_generation;
    to.block_network_ids_are_hashes = from.block_network_ids_are_hashes;
    to.network_permissions = from.network_permissions;
    to.server_configuration_join_info = ew::upgrade(from.server_configuration_join_info);
    to.server_telemetry_data = std::move(from.server_telemetry_data);
    return to;
}

} // namespace endweave
