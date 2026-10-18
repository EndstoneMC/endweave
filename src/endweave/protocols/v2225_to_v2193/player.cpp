#include "player.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::PlayerListPacket_<2225>::AddEntry, bp::PlayerListPacket_<2193>::AddEntry>::transform(
    Context<bp::PlayerListPacket_<2193>::AddEntry> &ctx, bp::PlayerListPacket_<2225>::AddEntry &&from)
{
    auto play_fab_id = std::move(from.play_fab_id);
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    to.skin.play_fab_id = std::move(play_fab_id);
}

} // namespace endweave
