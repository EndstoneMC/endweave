#include "endweave/protocols/v2168/input.h"

#include "endweave/protocols/v1001/inventory.h"
#include "endweave/protocols/v2168/inventory.h"

#include <bedrock/protocol/enum.hpp>
#include <cstddef>
#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::PlayerBlockActionData_<2168>, bp::PlayerBlockActionData_<1001>>::transform(
    Context<bp::PlayerBlockActionData_<1001>> &ctx, bp::PlayerBlockActionData_<2168> &&from)
{
    auto &to = ctx.out();
    // ENDWEAVE: INTERNAL_UPDATE collides with 1001's COUNT sentinel, so it maps to UNKNOWN,
    // which BDS ignores. Dropping the action would be truer, but a transform is one-to-one.
    to.player_action_type = from.player_action_type == bp::PlayerActionType_<2168>::INTERNAL_UPDATE
                              ? bp::PlayerActionType_<1001>::UNKNOWN
                              : static_cast<bp::PlayerActionType_<1001>>(from.player_action_type);
    to.pos = from.pos;
    to.facing = from.facing;
}

void Transformer<bp::PlayerActionPacket_<2168>, bp::PlayerActionPacket_<1001>>::transform(
    Context<bp::PlayerActionPacket_<1001>> &ctx, bp::PlayerActionPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.runtime_id = from.runtime_id;
    // ENDWEAVE: INTERNAL_UPDATE is 38, which is 1001's COUNT sentinel, so passing it through would
    // hand the server an action off the end of its enum. UNKNOWN is the one value BDS ignores.
    to.action = from.action == bp::PlayerActionType_<2168>::INTERNAL_UPDATE
                  ? bp::PlayerActionType_<1001>::UNKNOWN
                  : static_cast<bp::PlayerActionType_<1001>>(from.action);
    to.pos = from.pos;
    to.result_pos = from.result_pos;
    to.face = from.face;
}

void Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2168>,
                 bp::PackedItemUseLegacyInventoryTransaction_<1001>>::
    transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<1001>> &ctx,
              bp::PackedItemUseLegacyInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.id = from.id.raw_id;
    // ENDWEAVE: 1001 reads the slots off the id rather than a flag, so a list the id does not gate
    // is one the server would never read back.
    if (from.slots.has_value()) {
        to.slots = std::move(from.slots).value();
    }
    to.transaction =
        ew::transform(ctx, ew::transform_to<bp::ItemUseInventoryTransaction_<1001>>(ctx, std::move(from.transaction)));
}

void Transformer<bp::PlayerAuthInputPacket_<2168>, bp::PlayerAuthInputPacket_<1001>>::transform(
    Context<bp::PlayerAuthInputPacket_<1001>> &ctx, bp::PlayerAuthInputPacket_<2168> &&from)
{
    using To = bp::PlayerAuthInputPacket_<1001>;

    auto &to = ctx.out();
    to.rot = from.rot;
    to.pos = from.pos;
    to.move = from.move;
    to.y_head_rot = from.y_head_rot;
    // ENDWEAVE: 2168 names the flags it set, 1001 wants them as bits. INTERNAL_UPDATE is 65, past
    // the end of 1001's bitset, so it is dropped along with anything else off the end.
    for (const auto flag : from.input_data) {
        const auto bit = static_cast<std::size_t>(flag);
        if (bit < to.input_data.size()) {
            to.input_data.set(bit);
        }
    }
    // ENDWEAVE: The gate flags are recomputed from the engaged payloads; at 1001 the flag drives
    // the reader, and one that disagrees eats the rest of the packet.
    to.input_data.set(static_cast<std::size_t>(To::InputData::PERFORM_ITEM_INTERACTION),
                      from.item_use_transaction.has_value());
    to.input_data.set(static_cast<std::size_t>(To::InputData::PERFORM_ITEM_STACK_REQUEST),
                      from.item_stack_request.has_value());
    to.input_data.set(static_cast<std::size_t>(To::InputData::PERFORM_BLOCK_ACTIONS),
                      from.player_block_actions.has_value());
    to.input_data.set(static_cast<std::size_t>(To::InputData::IS_IN_CLIENT_PREDICTED_VEHICLE),
                      from.vehicle_rot.has_value() || from.client_predicted_vehicle.has_value());
    to.input_mode = from.input_mode;
    to.play_mode = from.play_mode;
    to.new_interaction_model = from.new_interaction_model;
    to.interact_rotation = from.interact_rotation;
    to.client_tick = from.client_tick;
    to.pos_delta = from.pos_delta;
    if (from.item_use_transaction.has_value()) {
        to.item_use_transaction = ew::transform(ctx, std::move(from.item_use_transaction.value()));
    }
    if (from.item_stack_request.has_value()) {
        to.item_stack_request = ew::transform(ctx, std::move(from.item_stack_request.value()));
    }
    if (from.player_block_actions.has_value()) {
        to.player_block_actions = ew::transform(ctx, std::move(from.player_block_actions.value()));
    }
    // ENDWEAVE: 1001 cannot say "absent" here, but the gate flag is clear, so the zeroes never
    // reach the wire.
    to.vehicle_rot = from.vehicle_rot.value_or(bp::Vec2{});
    to.client_predicted_vehicle = from.client_predicted_vehicle.value_or(bp::ActorUniqueID{});
    to.analog_move_vector = from.analog_move_vector;
    to.camera_orientation = from.camera_orientation;
    to.raw_move_vector = from.raw_move_vector;
}

void Transformer<bp::PlayerAuthInputPacket_<2168>::InputData, bp::PlayerAuthInputPacket_<2192>::InputData>::transform(
    Context<bp::PlayerAuthInputPacket_<2192>::InputData> &ctx, bp::PlayerAuthInputPacket_<2168>::InputData &&from)
{
    using To = bp::PlayerAuthInputPacket_<2192>::InputData;
    ctx.out() = bp::enum_cast<To>(bp::enum_name(from)).value_or(To::INPUT_NUM);
}

void Transformer<bp::PackedItemUseLegacyInventoryTransaction_<2168>,
                 bp::PackedItemUseLegacyInventoryTransaction_<2192>>::
    transform(Context<bp::PackedItemUseLegacyInventoryTransaction_<2192>> &ctx,
              bp::PackedItemUseLegacyInventoryTransaction_<2168> &&from)
{
    auto &to = ctx.out();
    to.id = from.id;
    to.slots = std::move(from.slots);
    to.transaction = ew::transform(ctx, std::move(from.transaction));
}

void Transformer<bp::PlayerAuthInputPacket_<2168>, bp::PlayerAuthInputPacket_<2192>>::transform(
    Context<bp::PlayerAuthInputPacket_<2192>> &ctx, bp::PlayerAuthInputPacket_<2168> &&from)
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
