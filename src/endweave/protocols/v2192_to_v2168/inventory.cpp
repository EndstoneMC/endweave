#include "endweave/protocols/v2192_to_v2168/inventory.h"

#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/transaction.h>
#include <utility>
#include <variant>

namespace ew = endweave;

namespace endweave {

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

void Transformer<bp::ItemUseInventoryTransaction_<2192>::ActionType,
                 bp::ItemUseInventoryTransaction_<2168>::ActionType>::
    transform(Context<bp::ItemUseInventoryTransaction_<2168>::ActionType> &ctx,
              bp::ItemUseInventoryTransaction_<2192>::ActionType &&from)
{
    using To = bp::ItemUseInventoryTransaction_<2168>::ActionType;
    const auto action = bp::enum_cast<To>(bp::enum_name(from));
    if (!action) {
        ctx.cancel();
        return;
    }
    ctx.out() = action.value();
}

void Transformer<bp::ContainerOpenPacket_<2192>, bp::ContainerOpenPacket_<2168>>::transform(
    Context<bp::ContainerOpenPacket_<2168>> &ctx, bp::ContainerOpenPacket_<2192> &&from)
{
    // ENDWEAVE: a container 2168 does not name has no screen there, and opening the wrong one is
    // further from what the server meant than opening nothing.
    const auto type = bp::enum_cast<bp::ContainerType_<2168>>(bp::enum_name(from.type));
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

void Transformer<bp::ContainerClosePacket_<2192>, bp::ContainerClosePacket_<2168>>::transform(
    Context<bp::ContainerClosePacket_<2168>> &ctx, bp::ContainerClosePacket_<2192> &&from)
{
    const auto type = bp::enum_cast<bp::ContainerType_<2168>>(bp::enum_name(from.container_type));
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
