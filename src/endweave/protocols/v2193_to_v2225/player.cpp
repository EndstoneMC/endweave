#include "player.h"

#include <optional>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::AddPlayerPacket_<2193>, bp::AddPlayerPacket_<2225>>::transform(
    Context<bp::AddPlayerPacket_<2225>> &ctx, bp::AddPlayerPacket_<2193> &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: 2193 has no player riding a block, so every player spawns on none.
    to.passenger_block_data = std::nullopt;
}

void Transformer<bp::PlayerListPacket_<2193>::AddEntry, bp::PlayerListPacket_<2225>::AddEntry>::transform(
    Context<bp::PlayerListPacket_<2225>::AddEntry> &ctx, bp::PlayerListPacket_<2193>::AddEntry &&from)
{
    auto &to = ctx.out();
    to.play_fab_id = std::move(from.skin.play_fab_id);
    ew::transform_members(ctx, std::move(from), to);
}

} // namespace endweave
