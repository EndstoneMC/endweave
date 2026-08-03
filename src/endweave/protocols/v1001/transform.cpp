#include "endweave/protocols/v1001/transform.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::v2168::SerializedNetworkItemStackDescriptor Transformer<bp::v1001::SerializedNetworkItemStackDescriptor>::transform(
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

bp::v2168::InventoryContentPacket Transformer<bp::v1001::InventoryContentPacket>::transform(
    bp::v1001::InventoryContentPacket &&from)
{
    bp::v2168::InventoryContentPacket to;
    to.inventory_id = from.inventory_id;
    to.slots = ew::transform(from.slots);
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::transform(from.storage_item);
    return to;
}

bp::v2168::StartGamePacket Transformer<bp::v1001::StartGamePacket>::transform(bp::v1001::StartGamePacket &&from)
{
    bp::v2168::StartGamePacket to;
    to.entity_id = from.entity_id;
    to.runtime_id = from.runtime_id;
    to.entity_game_type = from.entity_game_type;
    to.pos = from.pos;
    to.rot = from.rot;
    to.settings = ew::transform(from.settings);
    to.level_id = std::move(from.level_id);
    to.level_name = std::move(from.level_name);
    to.template_content_identity = std::move(from.template_content_identity);
    to.is_trial = from.is_trial;
    to.movement_settings = from.movement_settings;
    to.level_current_time = static_cast<std::uint64_t>(from.level_current_time);
    to.enchantment_seed = from.enchantment_seed;
    to.block_properties = ew::transform(from.block_properties);
    to.multiplayer_correlation_id = std::move(from.multiplayer_correlation_id);
    to.enable_item_stack_net_manager = from.enable_item_stack_net_manager;
    to.server_version = std::move(from.server_version);
    to.player_property_data = std::move(from.player_property_data);
    to.server_block_type_registry_checksum = from.server_block_type_registry_checksum;
    to.world_template_id = from.world_template_id;
    to.server_enabled_client_side_generation = from.server_enabled_client_side_generation;
    to.block_network_ids_are_hashes = from.block_network_ids_are_hashes;
    to.network_permissions = from.network_permissions;
    to.server_configuration_join_info = ew::transform(from.server_configuration_join_info);
    to.server_telemetry_data = std::move(from.server_telemetry_data);
    return to;
}

} // namespace endweave
