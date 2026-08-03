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

} // namespace endweave
