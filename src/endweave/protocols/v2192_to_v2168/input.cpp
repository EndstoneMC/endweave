#include "endweave/protocols/v2192_to_v2168/input.h"

#include "endweave/protocols/v2192_to_v2168/inventory.h"

#include <bedrock/protocol/enum.hpp>
#include <bedrock/protocol/transaction.h>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::PlayerAuthInputPacket_<2192>::InputData, bp::PlayerAuthInputPacket_<2168>::InputData>::transform(
    Context<bp::PlayerAuthInputPacket_<2168>::InputData> &ctx, bp::PlayerAuthInputPacket_<2192>::InputData &&from)
{
    using To = bp::PlayerAuthInputPacket_<2168>::InputData;
    ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::InputNum);
}

void Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2192>,
                 bp::PackedItemUseLegacyInventoryTransaction_<2168>>::
    transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<2168>> &ctx,
              bp::PackedItemUseLegacyInventoryTransaction_<2192> &&from)
{
    auto &to = ctx.out();
    to.id = from.id;
    to.slots = std::move(from.slots);
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::PlayerAuthInputPacket_<2192>, bp::PlayerAuthInputPacket_<2168>>::transform(
    Context<bp::PlayerAuthInputPacket_<2168>> &ctx, bp::PlayerAuthInputPacket_<2192> &&from)
{
    auto &to = ctx.out();
    to.rot = from.rot;
    to.pos = from.pos;
    to.move = from.move;
    to.y_head_rot = from.y_head_rot;
    to.input_data = ew::transform(ctx, std::move(from.input_data));
    to.input_mode = from.input_mode;
    to.play_mode = from.play_mode;
    to.new_interaction_model = from.new_interaction_model;
    to.interact_rotation = from.interact_rotation;
    to.client_tick = from.client_tick;
    to.pos_delta = from.pos_delta;
    to.item_use_transaction = ew::transform(ctx, std::move(from.item_use_transaction));
    to.item_stack_request = std::move(from.item_stack_request);
    to.player_block_actions = std::move(from.player_block_actions);
    to.vehicle_rot = from.vehicle_rot;
    to.client_predicted_vehicle = from.client_predicted_vehicle;
    to.analog_move_vector = from.analog_move_vector;
    to.camera_orientation = from.camera_orientation;
    to.raw_move_vector = from.raw_move_vector;
}

} // namespace endweave
