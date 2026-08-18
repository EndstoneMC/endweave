#include "endweave/protocols/v2192/inventory.h"

#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::InventorySource_<2192>, bp::InventorySource_<2168>>::transform(
    Context<bp::InventorySource_<2168>> &ctx, bp::InventorySource_<2192> &&from)
{
    auto &to = ctx.out();
    to.type = from.type;
    to.container_id = from.container_id;
    to.flags = from.flags;
}

void Transformer<bp::InventoryAction_<2192>, bp::InventoryAction_<2168>>::transform(
    Context<bp::InventoryAction_<2168>> &ctx, bp::InventoryAction_<2192> &&from)
{
    auto &to = ctx.out();
    to.source = ew::transform(ctx, std::move(from.source));
    to.slot = from.slot;
    to.from_item_descriptor = std::move(from.from_item_descriptor);
    to.to_item_descriptor = std::move(from.to_item_descriptor);
}

void Transformer<bp::InventoryTransaction_<2192>, bp::InventoryTransaction_<2168>>::transform(
    Context<bp::InventoryTransaction_<2168>> &ctx, bp::InventoryTransaction_<2192> &&from)
{
    auto &to = ctx.out();
    to.actions = ew::transform(ctx, std::move(from.actions));
}

void Transformer<bp::NormalTransactionData_<2192>, bp::NormalTransactionData_<2168>>::transform(
    Context<bp::NormalTransactionData_<2168>> &ctx, bp::NormalTransactionData_<2192> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::InventoryMismatchData_<2192>, bp::InventoryMismatchData_<2168>>::transform(
    Context<bp::InventoryMismatchData_<2168>> &ctx, bp::InventoryMismatchData_<2192> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::ItemUseInventoryTransaction_<2192>, bp::ItemUseInventoryTransaction_<2168>>::transform(
    Context<bp::ItemUseInventoryTransaction_<2168>> &ctx, bp::ItemUseInventoryTransaction_<2192> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    // ENDWEAVE: hand is dropped; 2168 has no field for it, so an off-hand use reaches the
    // server as a main-hand one.
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
}

void Transformer<bp::ItemUseOnActorInventoryTransaction_<2192>, bp::ItemUseOnActorInventoryTransaction_<2168>>::
    transform(Context<bp::ItemUseOnActorInventoryTransaction_<2168>> &ctx,
              bp::ItemUseOnActorInventoryTransaction_<2192> &&from)
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

void Transformer<bp::ItemReleaseInventoryTransaction_<2192>, bp::ItemReleaseInventoryTransaction_<2168>>::transform(
    Context<bp::ItemReleaseInventoryTransaction_<2168>> &ctx, bp::ItemReleaseInventoryTransaction_<2192> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
}

void Transformer<bp::TransactionData_<2192>, bp::TransactionData_<2168>>::transform(
    Context<bp::TransactionData_<2168>> &ctx, bp::TransactionData_<2192> &&from)
{
    auto &to = ctx.out();
    std::visit(
        [&to, &ctx](auto &data) {
            to = ew::transform(ctx, std::move(data));
        },
        from);
}

void Transformer<bp::InventoryTransactionPacket_<2192>, bp::InventoryTransactionPacket_<2168>>::transform(
    Context<bp::InventoryTransactionPacket_<2168>> &ctx, bp::InventoryTransactionPacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.legacy_request_id = from.legacy_request_id;
    to.legacy_set_item_slots = std::move(from.legacy_set_item_slots);
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

} // namespace endweave
