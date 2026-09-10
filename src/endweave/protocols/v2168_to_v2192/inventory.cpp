#include "inventory.h"

#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/item.h>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::ItemUseInventoryTransaction_<2168>::ActionType,
                 bp::ItemUseInventoryTransaction_<2192>::ActionType>::
    transform(Context<bp::ItemUseInventoryTransaction_<2192>::ActionType> &ctx,
              bp::ItemUseInventoryTransaction_<2168>::ActionType &&from)
{
    using To = bp::ItemUseInventoryTransaction_<2192>::ActionType;
    const auto action = bp::enum_cast<To>(bp::enum_name(from));
    if (!action) {
        ctx.cancel();
        return;
    }
    ctx.out() = action.value();
}

void Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<2192>>::transform(
    Context<bp::ItemUseInventoryTransaction_<2192>> &ctx, bp::ItemUseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = ew::transform(ctx, std::move(from.action_type));
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    // ENDWEAVE: hand is invented as the main hand; 2168 has no field for it, so an off-hand
    // use by an older client reaches a 2192 server as a main-hand one.
    to.hand = bp::HandSlot::Mainhand;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
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
