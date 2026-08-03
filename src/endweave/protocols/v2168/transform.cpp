#include "endweave/protocols/v2168/transform.h"

#include <utility>

namespace endweave {

bp::v1001::SerializedNetworkItemStackDescriptor Transformer<bp::v2168::SerializedNetworkItemStackDescriptor>::transform(
    bp::v2168::SerializedNetworkItemStackDescriptor &&from)
{
    bp::v1001::SerializedNetworkItemStackDescriptor to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    if (from.net_id_variant.has_value()) {
        const auto net_id = from.net_id_variant.value();
        if (net_id >= 0) {
            to.net_id_variant = bp::ItemStackNetId{net_id};
        }
        else if (net_id % 2 != 0) {
            to.net_id_variant = bp::ItemStackRequestId{(-net_id - 1) / 2};
        }
        else {
            to.net_id_variant = bp::ItemStackLegacyRequestId{-net_id / 2};
        }
    }
    to.block_runtime_id = from.block_runtime_id;
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

bp::v1001::InventoryContentPacket Transformer<bp::v2168::InventoryContentPacket>::transform(
    bp::v2168::InventoryContentPacket &&from)
{
    using Item = Transformer<bp::v2168::SerializedNetworkItemStackDescriptor>;

    bp::v1001::InventoryContentPacket to;
    to.inventory_id = from.inventory_id;
    to.slots.reserve(from.slots.size());
    for (auto &slot : from.slots) {
        to.slots.push_back(Item::transform(std::move(slot)));
    }
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = Item::transform(std::move(from.storage_item));
    return to;
}

bp::v1001::StartGamePacket Transformer<bp::v2168::StartGamePacket>::transform(bp::v2168::StartGamePacket &&from)
{
    bp::v1001::StartGamePacket to;
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.entity_game_type = from.entity_game_type;
    to.pos = from.pos;
    to.rot = from.rot;
    to.settings = Transformer<bp::v2168::LevelSettings>::transform(std::move(from.settings));
    to.level_id = std::move(from.level_id);
    to.level_name = std::move(from.level_name);
    to.template_content_identity = std::move(from.template_content_identity);
    to.is_trial = from.is_trial;
    to.movement_settings = from.movement_settings;
    to.level_current_time = static_cast<std::int64_t>(from.level_current_time);
    to.enchantment_seed = from.enchantment_seed;
    to.block_properties.reserve(from.block_properties.size());
    for (auto &property : from.block_properties) {
        to.block_properties.push_back(Transformer<bp::v2168::ServerBlockProperty>::transform(std::move(property)));
    }
    to.multiplayer_correlation_id = std::move(from.multiplayer_correlation_id);
    to.enable_item_stack_net_manager = from.enable_item_stack_net_manager;
    to.server_version = std::move(from.server_version);
    to.player_property_data = std::move(from.player_property_data);
    to.server_block_type_registry_checksum = from.server_block_type_registry_checksum;
    to.world_template_id = from.world_template_id;
    to.server_enabled_client_side_generation = from.server_enabled_client_side_generation;
    to.block_network_ids_are_hashes = from.block_network_ids_are_hashes;
    to.network_permissions = from.network_permissions;
    to.is_chat_logging = false;
    if (from.server_configuration_join_info.has_value()) {
        to.server_configuration_join_info = Transformer<bp::v2168::ServerConfigurationJoinInfo>::transform(
            std::move(from.server_configuration_join_info.value()));
    }
    to.server_telemetry_data = std::move(from.server_telemetry_data);
    return to;
}

} // namespace endweave
