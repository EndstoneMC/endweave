#include "boss.h"

#include <utility>

namespace endweave {

void Transformer<bp::BossEventPacket_<2192>, bp::BossEventPacket_<2168>>::transform(
    Context<bp::BossEventPacket_<2168>> &ctx, bp::BossEventPacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.boss_id = from.boss_id;
    // ENDWEAVE: player_id has no source at 2192, which dropped it, so it is invented as the
    // null actor; a 2168 client that keys the bar on the id is handed one no actor holds.
    to.player_id = {};
    to.event_type = from.event_type;
    to.name = std::move(from.name);
    to.filtered_name = std::move(from.filtered_name);
    to.health_percent = from.health_percent;
    to.color = from.color;
    to.overlay = from.overlay;
}

} // namespace endweave
