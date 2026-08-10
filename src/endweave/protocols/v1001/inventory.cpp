#include "endweave/protocols/v1001/inventory.h"

#include <cstdint>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

bp::SerializedNetworkItemStackDescriptor_<2168> Transformer<
    bp::NetworkItemStackDescriptor,
    bp::SerializedNetworkItemStackDescriptor_<2168>>::transform(bp::NetworkItemStackDescriptor &&from)
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
        to.net_id_variant = from.net_id.value().raw_id;
    }
    to.block_runtime_id = static_cast<std::uint32_t>(from.block_runtime_id);
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

bp::SerializedNetworkItemStackDescriptor_<2168> Transformer<
    bp::SerializedNetworkItemStackDescriptor_<1001>,
    bp::SerializedNetworkItemStackDescriptor_<2168>>::transform(bp::SerializedNetworkItemStackDescriptor_<1001> &&from)
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
            to.net_id_variant = -2 * request_id->raw_id - 1;
        }
        else if (const auto *legacy_id = std::get_if<bp::ItemStackLegacyRequestId>(&net_id)) {
            to.net_id_variant = -2 * legacy_id->raw_id;
        }
        else {
            to.net_id_variant = std::get<bp::ItemStackNetId>(net_id).raw_id;
        }
    }
    to.block_runtime_id = from.block_runtime_id;
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

bp::InventoryAction_<2168> Transformer<bp::InventoryAction_<1001>, bp::InventoryAction_<2168>>::transform(
    bp::InventoryAction_<1001> &&from)
{
    bp::InventoryAction_<2168> to;
    to.source = from.source;
    to.slot = from.slot;
    to.from_item_descriptor = ew::transform(std::move(from.from_item_descriptor));
    to.to_item_descriptor = ew::transform(std::move(from.to_item_descriptor));
    return to;
}

bp::InventoryTransaction_<2168> Transformer<
    bp::InventoryTransaction_<1001>, bp::InventoryTransaction_<2168>>::transform(bp::InventoryTransaction_<1001> &&from)
{
    bp::InventoryTransaction_<2168> to;
    to.actions = ew::transform(std::move(from.actions));
    return to;
}

bp::NormalTransactionData_<2168> Transformer<bp::NormalTransactionData_<1001>, bp::NormalTransactionData_<2168>>::
    transform(bp::NormalTransactionData_<1001> &&from)
{
    bp::NormalTransactionData_<2168> to;
    to.transaction = ew::transform(std::move(from.transaction));
    return to;
}

bp::InventoryMismatchData_<2168> Transformer<bp::InventoryMismatchData_<1001>, bp::InventoryMismatchData_<2168>>::
    transform(bp::InventoryMismatchData_<1001> &&from)
{
    bp::InventoryMismatchData_<2168> to;
    to.transaction = ew::transform(std::move(from.transaction));
    return to;
}

bp::ItemUseInventoryTransaction_<2168> Transformer<
    bp::ItemUseInventoryTransaction_<1001>,
    bp::ItemUseInventoryTransaction_<2168>>::transform(bp::ItemUseInventoryTransaction_<1001> &&from)
{
    bp::ItemUseInventoryTransaction_<2168> to;
    to.transaction = ew::transform(std::move(from.transaction));
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    to.item = ew::transform(std::move(from.item));
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
    return to;
}

bp::ItemUseInventoryTransaction_<1001> Transformer<
    bp::legacy::ItemUseInventoryTransaction_<1001>,
    bp::ItemUseInventoryTransaction_<1001>>::transform(bp::legacy::ItemUseInventoryTransaction_<1001> &&from)
{
    bp::ItemUseInventoryTransaction_<1001> to;
    // ENDWEAVE: packet 144 did not cerealise until 2168, so its transaction writes the action list
    // bare where the cerealised one puts a member-present marker in front of it.
    to.transaction.actions = std::move(from.actions);
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    // ENDWEAVE: BDS holds the face as a FacingID, which is a byte; only the pre-cereal write
    // widened it to a varint.
    to.face = static_cast<std::uint8_t>(from.face);
    to.slot = from.slot;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
    return to;
}

bp::legacy::ItemUseInventoryTransaction_<1001> Transformer<
    bp::ItemUseInventoryTransaction_<1001>,
    bp::legacy::ItemUseInventoryTransaction_<1001>>::transform(bp::ItemUseInventoryTransaction_<1001> &&from)
{
    bp::legacy::ItemUseInventoryTransaction_<1001> to;
    to.actions = std::move(from.transaction.actions);
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
    return to;
}

bp::ItemUseOnActorInventoryTransaction_<2168> Transformer<
    bp::ItemUseOnActorInventoryTransaction_<1001>,
    bp::ItemUseOnActorInventoryTransaction_<2168>>::transform(bp::ItemUseOnActorInventoryTransaction_<1001> &&from)
{
    bp::ItemUseOnActorInventoryTransaction_<2168> to;
    to.transaction = ew::transform(std::move(from.transaction));
    to.runtime_id = from.runtime_id;
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::transform(std::move(from.item));
    to.from_pos = from.from_pos;
    to.hit_pos = from.hit_pos;
    return to;
}

bp::ItemReleaseInventoryTransaction_<2168> Transformer<
    bp::ItemReleaseInventoryTransaction_<1001>,
    bp::ItemReleaseInventoryTransaction_<2168>>::transform(bp::ItemReleaseInventoryTransaction_<1001> &&from)
{
    bp::ItemReleaseInventoryTransaction_<2168> to;
    to.transaction = ew::transform(std::move(from.transaction));
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::transform(std::move(from.item));
    to.from_pos = from.from_pos;
    return to;
}

bp::TransactionData_<2168> Transformer<bp::TransactionData_<1001>, bp::TransactionData_<2168>>::transform(
    bp::TransactionData_<1001> &&from)
{
    bp::TransactionData_<2168> to;
    std::visit(
        [&to](auto &data) {
            to = ew::transform(std::move(data));
        },
        from);
    return to;
}

bp::InventoryTransactionPacket_<2168> Transformer<
    bp::InventoryTransactionPacket_<1001>,
    bp::InventoryTransactionPacket_<2168>>::transform(bp::InventoryTransactionPacket_<1001> &&from)
{
    bp::InventoryTransactionPacket_<2168> to;
    to.legacy_request_id = from.legacy_request_id;
    to.legacy_set_item_slots = std::move(from.legacy_set_item_slots);
    to.transaction = ew::transform(std::move(from.transaction));
    return to;
}

bp::InventoryContentPacket_<2168> Transformer<bp::InventoryContentPacket_<1001>, bp::InventoryContentPacket_<2168>>::
    transform(bp::InventoryContentPacket_<1001> &&from)
{
    bp::InventoryContentPacket_<2168> to;
    to.inventory_id = from.inventory_id;
    to.slots = ew::transform(std::move(from.slots));
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::transform(std::move(from.storage_item));
    return to;
}

bp::InventorySlotPacket_<2168> Transformer<bp::InventorySlotPacket_<1001>, bp::InventorySlotPacket_<2168>>::transform(
    bp::InventorySlotPacket_<1001> &&from)
{
    bp::InventorySlotPacket_<2168> to;
    to.inventory_id = from.inventory_id;
    to.slot = from.slot;
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::transform(std::move(from.storage_item));
    to.item = ew::transform(std::move(from.item));
    return to;
}

} // namespace endweave
