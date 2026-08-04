#include "endweave/protocols/v2168/inventory.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::SerializedNetworkItemStackDescriptor_<1001> Transformer<bp::SerializedNetworkItemStackDescriptor_<2168>>::downgrade(
    bp::SerializedNetworkItemStackDescriptor_<2168> &&from)
{
    bp::SerializedNetworkItemStackDescriptor_<1001> to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    // ENDWEAVE: reads the 1001 case back off sign and parity; round-trips with the upgrade.
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

bp::InventoryAction_<1001> Transformer<bp::InventoryAction_<2168>>::downgrade(bp::InventoryAction_<2168> &&from)
{
    bp::InventoryAction_<1001> to;
    to.source = from.source;
    to.slot = from.slot;
    to.from_item = ew::downgrade(from.from_item);
    to.to_item = ew::downgrade(from.to_item);
    return to;
}

bp::InventoryTransaction_<1001> Transformer<bp::InventoryTransaction_<2168>>::downgrade(
    bp::InventoryTransaction_<2168> &&from)
{
    bp::InventoryTransaction_<1001> to;
    to.actions = ew::downgrade(from.actions);
    return to;
}

bp::NormalTransactionData_<1001> Transformer<bp::NormalTransactionData_<2168>>::downgrade(
    bp::NormalTransactionData_<2168> &&from)
{
    bp::NormalTransactionData_<1001> to;
    to.actions = ew::downgrade(from.actions);
    return to;
}

bp::InventoryMismatchData_<1001> Transformer<bp::InventoryMismatchData_<2168>>::downgrade(
    bp::InventoryMismatchData_<2168> &&from)
{
    bp::InventoryMismatchData_<1001> to;
    to.actions = ew::downgrade(from.actions);
    return to;
}

bp::ItemUseInventoryTransaction_<1001> Transformer<bp::ItemUseInventoryTransaction_<2168>>::downgrade(
    bp::ItemUseInventoryTransaction_<2168> &&from)
{
    bp::ItemUseInventoryTransaction_<1001> to;
    to.actions = ew::downgrade(from.actions);
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    to.item = ew::downgrade(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
    return to;
}

bp::ItemUseOnActorInventoryTransaction_<1001> Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>>::downgrade(
    bp::ItemUseOnActorInventoryTransaction_<2168> &&from)
{
    bp::ItemUseOnActorInventoryTransaction_<1001> to;
    to.actions = ew::downgrade(from.actions);
    to.target_runtime_id = from.target_runtime_id;
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::downgrade(from.item);
    to.from_pos = from.from_pos;
    to.hit_pos = from.hit_pos;
    return to;
}

bp::ItemReleaseInventoryTransaction_<1001> Transformer<bp::ItemReleaseInventoryTransaction_<2168>>::downgrade(
    bp::ItemReleaseInventoryTransaction_<2168> &&from)
{
    bp::ItemReleaseInventoryTransaction_<1001> to;
    to.actions = ew::downgrade(from.actions);
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::downgrade(from.item);
    to.from_pos = from.from_pos;
    return to;
}

bp::TransactionData_<1001> Transformer<bp::TransactionData_<2168>>::downgrade(bp::TransactionData_<2168> &&from)
{
    bp::TransactionData_<1001> to;
    std::visit(
        [&to](auto &data) {
            to = ew::downgrade(data);
        },
        from);
    return to;
}

bp::InventoryTransactionPacket_<1001> Transformer<bp::InventoryTransactionPacket_<2168>>::downgrade(
    bp::InventoryTransactionPacket_<2168> &&from)
{
    bp::InventoryTransactionPacket_<1001> to;
    to.legacy_request_id = from.legacy_request_id;
    to.legacy_set_item_slots = std::move(from.legacy_set_item_slots);
    to.transaction = ew::downgrade(from.transaction);
    return to;
}

bp::InventoryContentPacket_<1001> Transformer<bp::InventoryContentPacket_<2168>>::downgrade(
    bp::InventoryContentPacket_<2168> &&from)
{
    bp::InventoryContentPacket_<1001> to;
    to.inventory_id = from.inventory_id;
    to.slots = ew::downgrade(from.slots);
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::downgrade(from.storage_item);
    return to;
}

bp::InventorySlotPacket_<1001> Transformer<bp::InventorySlotPacket_<2168>>::downgrade(
    bp::InventorySlotPacket_<2168> &&from)
{
    bp::InventorySlotPacket_<1001> to;
    to.inventory_id = from.inventory_id;
    to.slot = from.slot;
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::downgrade(from.storage_item);
    to.item = ew::downgrade(from.item);
    return to;
}

} // namespace endweave
