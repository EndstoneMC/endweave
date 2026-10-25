#include "inventory.h"

#include <bedrock/protocol/item.h>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::ItemUseInventoryTransaction_<2193>, bp::ItemUseInventoryTransaction_<2168>>::transform(
    Context<bp::ItemUseInventoryTransaction_<2168>> &ctx, bp::ItemUseInventoryTransaction_<2193> &&from)
{
    // ENDWEAVE: 2168 has no hand to name, and an off-hand use carried as a main-hand one would act with
    // the off-hand item from the main hand's slot, so it is dropped.
    if (from.hand != bp::HandSlot::Mainhand) {
        ctx.cancel();
        return;
    }
    ew::transform_members(ctx, std::move(from), ctx.out());
}

} // namespace endweave
