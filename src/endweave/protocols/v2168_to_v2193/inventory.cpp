#include "inventory.h"

#include <bedrock/protocol/item.h>
#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::ItemUseInventoryTransaction_<2168>, bp::ItemUseInventoryTransaction_<2193>>::transform(
    Context<bp::ItemUseInventoryTransaction_<2193>> &ctx, bp::ItemUseInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.transaction = ew::transform(ctx, std::move(from.transaction));
    to.action_type = ew::transform(ctx, std::move(from.action_type));
    to.trigger_type = from.trigger_type;
    to.pos = from.pos;
    to.face = from.face;
    to.slot = from.slot;
    // ENDWEAVE: hand is invented as the main hand; 2168 has no field for it, so an off-hand
    // use by an older client reaches a 2193 server as a main-hand one.
    to.hand = bp::HandSlot::Mainhand;
    to.item = std::move(from.item);
    to.from_pos = from.from_pos;
    to.click_pos = from.click_pos;
    to.target_block_id = from.target_block_id;
    to.client_predicted_result = from.client_predicted_result;
    to.client_cooldown_state = from.client_cooldown_state;
}

} // namespace endweave
