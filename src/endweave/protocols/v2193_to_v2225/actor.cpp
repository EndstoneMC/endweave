#include "actor.h"

#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/item.h>
#include <optional>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::AddActorPacket_<2193>, bp::AddActorPacket_<2225>>::transform(
    Context<bp::AddActorPacket_<2225>> &ctx, bp::AddActorPacket_<2193> &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: 2193 has no actor riding a block, so every actor spawns on none.
    to.passenger_block_data = std::nullopt;
}

void Transformer<bp::AnimatePacket_<2193>, bp::AnimatePacket_<2225>>::transform(Context<bp::AnimatePacket_<2225>> &ctx,
                                                                                bp::AnimatePacket_<2193> &&from)
{
    using Action = bp::AnimatePacket_<2225>::Action;
    const auto action = bp::enum_cast<Action>(bp::enum_name(from.action));
    if (!action) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.action = *action;
    to.runtime_id = from.runtime_id;
    to.data = from.data;
    to.swing_source = from.swing_source;
    // ENDWEAVE: hand is invented as the main hand; 2193 never says which hand swung, so an off-hand
    // swing reaches 2225 as a main-hand one.
    to.hand = bp::HandSlot::Mainhand;
}

} // namespace endweave
