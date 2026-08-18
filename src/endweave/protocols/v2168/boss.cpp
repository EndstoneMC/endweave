#include "endweave/protocols/v2168/boss.h"

#include <utility>

namespace endweave {

void Transformer<bp::BossEventPacket_<2168>, bp::BossEventPacket_<2192>>::transform(
    Context<bp::BossEventPacket_<2192>> &ctx, bp::BossEventPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.boss_id = from.boss_id;
    // ENDWEAVE: player_id is dropped; 2192 removed it from the packet.
    to.event_type = from.event_type;
    to.name = std::move(from.name);
    to.filtered_name = std::move(from.filtered_name);
    to.health_percent = from.health_percent;
    to.color = from.color;
    to.overlay = from.overlay;
}

} // namespace endweave
