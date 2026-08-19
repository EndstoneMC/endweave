#include "endweave/protocols/v2192/inventory.h"

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

} // namespace endweave
