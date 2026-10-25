#include "input.h"

#include <bedrock/protocol/item.h>
#include <utility>
#include <vector>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::PlayerAuthInputPacket_<2193>, bp::PlayerAuthInputPacket_<2168>>::transform(
    Context<bp::PlayerAuthInputPacket_<2168>> &ctx, bp::PlayerAuthInputPacket_<2193> &&from)
{
    // ENDWEAVE: an off-hand use is dropped, as it is from an InventoryTransactionPacket, but the tick of
    // input it rides in is kept, so only the use and the flag that announces it are cleared.
    if (from.item_use_transaction && from.item_use_transaction->transaction.hand != bp::HandSlot::Mainhand) {
        from.item_use_transaction.reset();
        std::erase(from.input_data, bp::PlayerAuthInputPacket_<2193>::InputData::PerformItemInteraction);
    }
    ew::transform_members(ctx, std::move(from), ctx.out());
}

} // namespace endweave
