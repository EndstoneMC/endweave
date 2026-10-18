#include "inventory.h"

#include <bedrock/protocol/item.h>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::ItemUseOnActorInventoryTransaction_<2193>, bp::ItemUseOnActorInventoryTransaction_<2225>>::
    transform(Context<bp::ItemUseOnActorInventoryTransaction_<2225>> &ctx,
              bp::ItemUseOnActorInventoryTransaction_<2193> &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: hand is invented as the main hand; 2193 has no field for it, so an off-hand use on an
    // actor by an older client reaches a 2225 server as a main-hand one.
    to.hand = bp::HandSlot::Mainhand;
}

void Transformer<bp::ItemReleaseInventoryTransaction_<2193>, bp::ItemReleaseInventoryTransaction_<2225>>::transform(
    Context<bp::ItemReleaseInventoryTransaction_<2225>> &ctx, bp::ItemReleaseInventoryTransaction_<2193> &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: hand is invented as the main hand, as for an item used on an actor.
    to.hand = bp::HandSlot::Mainhand;
}

} // namespace endweave
