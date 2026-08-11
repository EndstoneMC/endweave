#include "endweave/protocols/v2168/inventory.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::SerializedNetworkItemStackDescriptor_<2168>, bp::SerializedNetworkItemStackDescriptor_<1001>>::
    transform(Context<bp::SerializedNetworkItemStackDescriptor_<1001>> &ctx,
              bp::SerializedNetworkItemStackDescriptor_<2168> &&from)
{
    auto &to = ctx.out();
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
}

void Transformer<bp::InventoryAction_<2168>, bp::InventoryAction_<1001>>::transform(
    Context<bp::InventoryAction_<1001>> &ctx, bp::InventoryAction_<2168> &&from)
{
    auto &to = ctx.out();
    to.source = from.source;
    to.slot = from.slot;
    to.from_item_descriptor = ew::transform(ctx, std::move(from.from_item_descriptor));
    to.to_item_descriptor = ew::transform(ctx, std::move(from.to_item_descriptor));
}

void Transformer<bp::InventoryTransaction_<2168>, bp::InventoryTransaction_<1001>>::transform(
    Context<bp::InventoryTransaction_<1001>> &ctx, bp::InventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.actions = ew::transform(ctx, std::move(from.actions));
}

void Transformer<bp::NormalTransactionData_<2168>, bp::NormalTransactionData_<1001>>::transform(
    Context<bp::NormalTransactionData_<1001>> &ctx, bp::NormalTransactionData_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::InventoryMismatchData_<2168>, bp::InventoryMismatchData_<1001>>::transform(
    Context<bp::InventoryMismatchData_<1001>> &ctx, bp::InventoryMismatchData_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<1001>>::transform(
    Context<bp::ItemUseInventoryTransaction_<1001>> &ctx, bp::ItemUseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    to.item = ew::transform(ctx, std::move(from.item));
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
}

void Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>, bp::ItemUseOnActorInventoryTransaction_<1001>>::
    transform(Context<bp::ItemUseOnActorInventoryTransaction_<1001>> &ctx,
              bp::ItemUseOnActorInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.runtime_id = from.runtime_id;
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::transform(ctx, std::move(from.item));
    to.from_pos = from.from_pos;
    to.hit_pos = from.hit_pos;
}

void Transformer<bp::ItemReleaseInventoryTransaction_<2168>, bp::ItemReleaseInventoryTransaction_<1001>>::transform(
    Context<bp::ItemReleaseInventoryTransaction_<1001>> &ctx, bp::ItemReleaseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = ew::transform(ctx, std::move(from.item));
    to.from_pos = from.from_pos;
}

void Transformer<bp::TransactionData_<2168>, bp::TransactionData_<1001>>::transform(
    Context<bp::TransactionData_<1001>> &ctx, bp::TransactionData_<2168> &&from)
{
    auto &to = ctx.out();
    std::visit(
        [&to, &ctx](auto &data) {
            to = ew::transform(ctx, std::move(data));
        },
        from);
}

void Transformer<bp::InventoryTransactionPacket_<2168>, bp::InventoryTransactionPacket_<1001>>::transform(
    Context<bp::InventoryTransactionPacket_<1001>> &ctx, bp::InventoryTransactionPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.legacy_request_id = from.legacy_request_id;
    to.legacy_set_item_slots = std::move(from.legacy_set_item_slots);
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::InventoryContentPacket_<2168>, bp::InventoryContentPacket_<1001>>::transform(
    Context<bp::InventoryContentPacket_<1001>> &ctx, bp::InventoryContentPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.inventory_id = from.inventory_id;
    to.slots = ew::transform(ctx, std::move(from.slots));
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::transform(ctx, std::move(from.storage_item));
}

void Transformer<bp::InventorySlotPacket_<2168>, bp::InventorySlotPacket_<1001>>::transform(
    Context<bp::InventorySlotPacket_<1001>> &ctx, bp::InventorySlotPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.inventory_id = from.inventory_id;
    to.slot = from.slot;
    to.full_container_name = std::move(from.full_container_name);
    to.storage_item = ew::transform(ctx, std::move(from.storage_item));
    to.item = ew::transform(ctx, std::move(from.item));
}

void Transformer<bp::InventorySource_<2168>, bp::InventorySource_<2187>>::transform(
    Context<bp::InventorySource_<2187>> &ctx, bp::InventorySource_<2168> &&from)
{
    auto &to = ctx.out();
    to.type = from.type;
    to.container_id = from.container_id;
    to.flags = from.flags;
}

void Transformer<bp::InventoryAction_<2168>, bp::InventoryAction_<2187>>::transform(
    Context<bp::InventoryAction_<2187>> &ctx, bp::InventoryAction_<2168> &&from)
{
    auto &to = ctx.out();
    to.source = ew::transform(ctx, std::move(from.source));
    to.slot = from.slot;
    to.from_item_descriptor = std::move(from.from_item_descriptor);
    to.to_item_descriptor = std::move(from.to_item_descriptor);
}

void Transformer<bp::InventoryTransaction_<2168>, bp::InventoryTransaction_<2187>>::transform(
    Context<bp::InventoryTransaction_<2187>> &ctx, bp::InventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.actions = ew::transform(ctx, std::move(from.actions));
}

void Transformer<bp::NormalTransactionData_<2168>, bp::NormalTransactionData_<2187>>::transform(
    Context<bp::NormalTransactionData_<2187>> &ctx, bp::NormalTransactionData_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::InventoryMismatchData_<2168>, bp::InventoryMismatchData_<2187>>::transform(
    Context<bp::InventoryMismatchData_<2187>> &ctx, bp::InventoryMismatchData_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<2187>>::transform(
    Context<bp::ItemUseInventoryTransaction_<2187>> &ctx, bp::ItemUseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
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
}

void Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>, bp::ItemUseOnActorInventoryTransaction_<2187>>::
    transform(Context<bp::ItemUseOnActorInventoryTransaction_<2187>> &ctx,
              bp::ItemUseOnActorInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.runtime_id = from.runtime_id;
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.hit_pos = from.hit_pos;
}

void Transformer<bp::ItemReleaseInventoryTransaction_<2168>, bp::ItemReleaseInventoryTransaction_<2187>>::transform(
    Context<bp::ItemReleaseInventoryTransaction_<2187>> &ctx, bp::ItemReleaseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
}

void Transformer<bp::TransactionData_<2168>, bp::TransactionData_<2187>>::transform(
    Context<bp::TransactionData_<2187>> &ctx, bp::TransactionData_<2168> &&from)
{
    auto &to = ctx.out();
    std::visit(
        [&to, &ctx](auto &data) {
            to = ew::transform(ctx, std::move(data));
        },
        from);
}

void Transformer<bp::InventoryTransactionPacket_<2168>, bp::InventoryTransactionPacket_<2187>>::transform(
    Context<bp::InventoryTransactionPacket_<2187>> &ctx, bp::InventoryTransactionPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.legacy_request_id = from.legacy_request_id;
    to.legacy_set_item_slots = std::move(from.legacy_set_item_slots);
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

} // namespace endweave
