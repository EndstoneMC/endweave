#include "actor.h"

#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/item.h>

namespace endweave {

void Transformer<bp::AnimatePacket_<2225>, bp::AnimatePacket_<2193>>::transform(Context<bp::AnimatePacket_<2193>> &ctx,
                                                                                bp::AnimatePacket_<2225> &&from)
{
    using Action = bp::AnimatePacket_<2193>::Action;
    const auto action = bp::enum_cast<Action>(bp::enum_name(from.action));
    // ENDWEAVE: 2193 has no hand to name, and an off-hand swing played as a main-hand one is further from
    // what happened than no swing at all.
    if (!action || from.hand != bp::HandSlot::Mainhand) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.action = *action;
    to.runtime_id = from.runtime_id;
    to.data = from.data;
    to.swing_source = from.swing_source;
}

} // namespace endweave
