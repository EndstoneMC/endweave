#include "inventory.h"

#include <bedrock/protocol/item.h>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::ItemUseOnActorInventoryTransaction_<2225>, bp::ItemUseOnActorInventoryTransaction_<2193>>::
    transform(Context<bp::ItemUseOnActorInventoryTransaction_<2193>> &ctx,
              bp::ItemUseOnActorInventoryTransaction_<2225> &&from)
{
    // ENDWEAVE: 2193 has no hand to name, and an off-hand use on an actor carried as a main-hand one would
    // act with the wrong item, so it is dropped.
    if (from.hand != bp::HandSlot::Mainhand) {
        ctx.cancel();
        return;
    }
    ew::transform_members(ctx, std::move(from), ctx.out());
}

void Transformer<bp::ItemReleaseInventoryTransaction_<2225>, bp::ItemReleaseInventoryTransaction_<2193>>::transform(
    Context<bp::ItemReleaseInventoryTransaction_<2193>> &ctx, bp::ItemReleaseInventoryTransaction_<2225> &&from)
{
    // ENDWEAVE: an off-hand release is dropped, as an off-hand use on an actor is.
    if (from.hand != bp::HandSlot::Mainhand) {
        ctx.cancel();
        return;
    }
    ew::transform_members(ctx, std::move(from), ctx.out());
}

} // namespace endweave
