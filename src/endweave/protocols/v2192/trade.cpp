#include "endweave/protocols/v2192/trade.h"

#include <bedrock/protocol/enum.hpp>
#include <utility>

namespace endweave {

void Transformer<bp::UpdateTradePacket_<2192>, bp::UpdateTradePacket_<2168>>::transform(
    Context<bp::UpdateTradePacket_<2168>> &ctx, bp::UpdateTradePacket_<2192> &&from)
{
    // ENDWEAVE: as in inventory.cpp -- a container 2168 does not name is not the trade screen the
    // server asked for.
    const auto type = bp::enum_cast<bp::ContainerType_<2168>>(bp::enum_name(from.type));
    if (!type) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.container_id = from.container_id;
    to.type = *type;
    to.size = from.size;
    to.trader_tier = from.trader_tier;
    to.entity_unique_id = from.entity_unique_id;
    to.last_trading_player = from.last_trading_player;
    to.display_name = std::move(from.display_name);
    to.use_new_trade_screen = from.use_new_trade_screen;
    to.using_economy_trade = from.using_economy_trade;
    to.data = std::move(from.data);
}

void Transformer<bp::UpdateEquipPacket_<2192>, bp::UpdateEquipPacket_<2168>>::transform(
    Context<bp::UpdateEquipPacket_<2168>> &ctx, bp::UpdateEquipPacket_<2192> &&from)
{
    const auto type = bp::enum_cast<bp::ContainerType_<2168>>(bp::enum_name(from.type));
    if (!type) {
        ctx.cancel();
        return;
    }
    auto &to = ctx.out();
    to.container_id = from.container_id;
    to.type = *type;
    to.size = from.size;
    to.entity_unique_id = from.entity_unique_id;
    to.data = std::move(from.data);
}

} // namespace endweave
