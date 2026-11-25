#include "actor.h"

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
    to.passenger_of_block_data = std::nullopt;
}

void Transformer<bp::AnimatePacket_<2193>, bp::AnimatePacket_<2225>>::transform(Context<bp::AnimatePacket_<2225>> &ctx,
                                                                                bp::AnimatePacket_<2193> &&from)
{
    auto &to = ctx.out();
    to.action = from.action;
    to.runtime_id = from.runtime_id;
    to.data = from.data;
    to.swing_source = from.swing_source;
    // ENDWEAVE: hand is invented as the main hand; 2193 never says which hand swung, so an off-hand
    // swing reaches 2225 as a main-hand one.
    to.hand_slot = bp::HandSlot::Mainhand;
}

} // namespace endweave
