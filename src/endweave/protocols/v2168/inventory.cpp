#include "endweave/protocols/v2168/inventory.h"

#include <bedrock/protocol/enum.hpp>
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

void Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<2192>>::transform(
    Context<bp::ItemUseInventoryTransaction_<2192>> &ctx, bp::ItemUseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    // ENDWEAVE: hand is invented as the main hand; 2168 has no field for it, so an off-hand
    // use by an older client reaches a 2192 server as a main-hand one.
    to.hand = bp::HandSlot::MAINHAND;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
}

void Transformer<bp::ItemUseOnActorInventoryTransaction_<2168>, bp::ItemUseOnActorInventoryTransaction_<2192>>::
    transform(Context<bp::ItemUseOnActorInventoryTransaction_<2192>> &ctx,
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

void Transformer<bp::ItemReleaseInventoryTransaction_<2168>, bp::ItemReleaseInventoryTransaction_<2192>>::transform(
    Context<bp::ItemReleaseInventoryTransaction_<2192>> &ctx, bp::ItemReleaseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = from.action_type;
    to.slot = from.slot;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
}

void Transformer<bp::TransactionData_<2168>, bp::TransactionData_<2192>>::transform(
    Context<bp::TransactionData_<2192>> &ctx, bp::TransactionData_<2168> &&from)
{
    auto &to = ctx.out();
    std::visit(
        [&to, &ctx](auto &data) {
            to = ew::transform(ctx, std::move(data));
        },
        from);
}

void Transformer<bp::ContainerOpenPacket_<2168>, bp::ContainerOpenPacket_<2192>>::transform(
    Context<bp::ContainerOpenPacket_<2192>> &ctx, bp::ContainerOpenPacket_<2168> &&from)
{
    // ENDWEAVE: 2192 names every container 2168 does, so this only answers a rename.
    const auto type = bp::enum_cast<bp::ContainerType_<2192>>(bp::enum_name(from.type));
    if (!type) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.container_id = from.container_id;
    to.type = *type;
    to.pos = from.pos;
    to.entity_unique_id = from.entity_unique_id;
}

void Transformer<bp::ContainerClosePacket_<2168>, bp::ContainerClosePacket_<2192>>::transform(
    Context<bp::ContainerClosePacket_<2192>> &ctx, bp::ContainerClosePacket_<2168> &&from)
{
    const auto type = bp::enum_cast<bp::ContainerType_<2192>>(bp::enum_name(from.container_type));
    if (!type) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.container_id = from.container_id;
    to.container_type = *type;
    to.server_initiated_close = from.server_initiated_close;
}

} // namespace endweave
