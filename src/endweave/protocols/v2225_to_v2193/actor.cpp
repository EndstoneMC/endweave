#include "actor.h"

#include <bedrock/protocol/item.h>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::AnimatePacket_<2225>, bp::AnimatePacket_<2193>>::transform(Context<bp::AnimatePacket_<2193>> &ctx,
                                                                                bp::AnimatePacket_<2225> &&from)
{
    // ENDWEAVE: 2193 has no hand to name, and an off-hand swing played as a main-hand one is further from
    // what happened than no swing at all.
    if (from.hand_slot != bp::HandSlot::Mainhand) {
        ctx.cancel();
        return;
    }
    ew::transform_members(ctx, std::move(from), ctx.out());
}

} // namespace endweave
