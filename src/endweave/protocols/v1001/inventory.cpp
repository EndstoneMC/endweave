#include "endweave/protocols/v1001/inventory.h"

#include <cstdint>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::SerializedNetworkItemStackDescriptor_<2168> Transformer<bp::NetworkItemStackDescriptor>::upgrade(
    bp::NetworkItemStackDescriptor &&from)
{
    bp::SerializedNetworkItemStackDescriptor_<2168> to;
    // ENDWEAVE: BDS cerealised its packets one at a time, so 12, 15 and 32 still send this
    // pre-cereal descriptor at 1001 while 31, 49 and 50 already send the cerealised one.
    to.id = static_cast<std::int16_t>(from.id);
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    // ENDWEAVE: a bare net id is the non-negative case of 2168's signed variant -- the pre-cereal
    // form has no request or legacy-request id to encode.
    if (from.net_id.has_value()) {
        to.net_id_variant = from.net_id.value().id;
    }
    to.block_runtime_id = static_cast<std::uint32_t>(from.block_runtime_id);
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

bp::SerializedNetworkItemStackDescriptor_<2168> Transformer<bp::SerializedNetworkItemStackDescriptor_<1001>>::upgrade(
    bp::SerializedNetworkItemStackDescriptor_<1001> &&from)
{
    bp::SerializedNetworkItemStackDescriptor_<2168> to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    // ENDWEAVE: 2168 has one signed varint where 1001 had a tagged union -- net id as-is, request
    // id as -2n-1, legacy request id as -2n.
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

bp::InventoryAction_<2168> Transformer<bp::InventoryAction_<1001>>::upgrade(bp::InventoryAction_<1001> &&from)
{
    bp::InventoryAction_<2168> to;
    to.source = from.source;
    to.slot = from.slot;
    to.from_item = ew::upgrade(from.from_item);
    to.to_item = ew::upgrade(from.to_item);
    return to;
}

bp::InventoryTransaction_<2168> Transformer<bp::InventoryTransaction_<1001>>::upgrade(
    bp::InventoryTransaction_<1001> &&from)
{
    bp::InventoryTransaction_<2168> to;
    to.actions = ew::upgrade(from.actions);
    return to;
}

bp::NormalTransactionData_<2168> Transformer<bp::NormalTransactionData_<1001>>::upgrade(
    bp::NormalTransactionData_<1001> &&from)
{
    bp::NormalTransactionData_<2168> to;
    to.actions = ew::upgrade(from.actions);
    return to;
}

bp::InventoryMismatchData_<2168> Transformer<bp::InventoryMismatchData_<1001>>::upgrade(
    bp::InventoryMismatchData_<1001> &&from)
{
    bp::InventoryMismatchData_<2168> to;
    to.actions = ew::upgrade(from.actions);
    return to;
}

bp::ItemUseInventoryTransaction_<2168> Transformer<bp::ItemUseInventoryTransaction_<1001>>::upgrade(
    bp::ItemUseInventoryTransaction_<1001> &&from)
{
    bp::ItemUseInventoryTransaction_<2168> to;
    to.actions = ew::upgrade(from.actions);
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    to.item = ew::upgrade(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
    return to;
}

bp::ItemUseOnActorInventoryTransaction_<2168> Transformer<bp::ItemUseOnActorInventoryTransaction_<1001>>::upgrade(
    bp::ItemUseOnActorInventoryTransaction_<1001> &&from)
{
    bp::ItemUseOnActorInventoryTransaction_<2168> to;
    to.actions = ew::upgrade(from.actions);
    to.target_runtime_id = from.target_runtime_id;
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::upgrade(from.item);
    to.from_pos = from.from_pos;
    to.hit_pos = from.hit_pos;
    return to;
}

bp::ItemReleaseInventoryTransaction_<2168> Transformer<bp::ItemReleaseInventoryTransaction_<1001>>::upgrade(
    bp::ItemReleaseInventoryTransaction_<1001> &&from)
{
    bp::ItemReleaseInventoryTransaction_<2168> to;
    to.actions = ew::upgrade(from.actions);
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::upgrade(from.item);
    to.from_pos = from.from_pos;
    return to;
}

bp::TransactionData_<2168> Transformer<bp::TransactionData_<1001>>::upgrade(bp::TransactionData_<1001> &&from)
{
    bp::TransactionData_<2168> to;
    std::visit(
        [&to](auto &data) {
            to = ew::upgrade(data);
        },
        from);
    return to;
}

bp::InventoryTransactionPacket_<2168> Transformer<bp::InventoryTransactionPacket_<1001>>::upgrade(
    bp::InventoryTransactionPacket_<1001> &&from)
{
    bp::InventoryTransactionPacket_<2168> to;
    to.legacy_request_id = from.legacy_request_id;
    to.legacy_set_item_slots = std::move(from.legacy_set_item_slots);
    to.transaction = ew::upgrade(from.transaction);
    return to;
}

bp::InventoryContentPacket_<2168> Transformer<bp::InventoryContentPacket_<1001>>::upgrade(
    bp::InventoryContentPacket_<1001> &&from)
{
    bp::InventoryContentPacket_<2168> to;
    to.inventory_id = from.inventory_id;
    to.slots = ew::upgrade(from.slots);
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::upgrade(from.storage_item);
    return to;
}

bp::InventorySlotPacket_<2168> Transformer<bp::InventorySlotPacket_<1001>>::upgrade(
    bp::InventorySlotPacket_<1001> &&from)
{
    bp::InventorySlotPacket_<2168> to;
    to.inventory_id = from.inventory_id;
    to.slot = from.slot;
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::upgrade(from.storage_item);
    to.item = ew::upgrade(from.item);
    return to;
}

} // namespace endweave
