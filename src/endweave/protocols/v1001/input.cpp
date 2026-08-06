#include "endweave/protocols/v1001/input.h"

#include "endweave/protocols/v1001/inventory.h"

#include <cstddef>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::PlayerBlockActionData_<2168> Transformer<bp::PlayerBlockActionData_<1001>, bp::PlayerBlockActionData_<2168>>::
    transform(bp::PlayerBlockActionData_<1001> &&from)
{
    bp::PlayerBlockActionData_<2168> to;
    // ENDWEAVE: 2168 only appends INTERNAL_UPDATE, so every action 1001 can name keeps its value.
    to.player_action_type = static_cast<bp::PlayerActionType_<2168>>(from.player_action_type);
    to.pos = from.pos;
    to.facing = from.facing;
    return to;
}

bp::PlayerActionPacket_<2168> Transformer<bp::PlayerActionPacket_<1001>, bp::PlayerActionPacket_<2168>>::transform(
    bp::PlayerActionPacket_<1001> &&from)
{
    bp::PlayerActionPacket_<2168> to;
    to.runtime_id = from.runtime_id;
    // ENDWEAVE: 2168 only appends INTERNAL_UPDATE, so every action 1001 can name keeps its value.
    to.action = static_cast<bp::PlayerActionType_<2168>>(from.action);
    to.pos = from.pos;
    to.result_pos = from.result_pos;
    to.face = from.face;
    return to;
}

bp::PackedItemUseLegacyInventoryTransaction_<2168> Transformer<bp::PackedItemUseLegacyInventoryTransaction_<1001>,
                                                               bp::PackedItemUseLegacyInventoryTransaction_<2168>>::
    transform(bp::PackedItemUseLegacyInventoryTransaction_<1001> &&from)
{
    bp::PackedItemUseLegacyInventoryTransaction_<2168> to;
    to.id = bp::ItemStackLegacyRequestId{from.id};
    // ENDWEAVE: 1001 reads the slots only under a negative even id and 2168 flags them, so the gate
    // the reader applied is what engages the flag.
    if (from.id < -1 && (from.id & 1) == 0) {
        to.slots = std::move(from.slots);
    }
    to.transaction =
        ew::transform(ew::transform_to<bp::ItemUseInventoryTransaction_<1001>>(std::move(from.transaction)));
    return to;
}

bp::PlayerAuthInputPacket_<2168> Transformer<bp::PlayerAuthInputPacket_<1001>, bp::PlayerAuthInputPacket_<2168>>::
    transform(bp::PlayerAuthInputPacket_<1001> &&from)
{
    using From = bp::PlayerAuthInputPacket_<1001>;

    bp::PlayerAuthInputPacket_<2168> to;
    to.rot = from.rot;
    to.pos = from.pos;
    to.move = from.move;
    to.y_head_rot = from.y_head_rot;
    // ENDWEAVE: 2168 only appends INTERNAL_UPDATE at 65, so every bit 1001 can set keeps its index
    // and the widening leaves the new one clear.
    for (std::size_t bit = 0; bit < from.input_data.size(); ++bit) {
        to.input_data.set(bit, from.input_data.test(bit));
    }
    to.input_mode = from.input_mode;
    to.play_mode = from.play_mode;
    to.new_interaction_model = from.new_interaction_model;
    to.interact_rotation = from.interact_rotation;
    to.client_tick = from.client_tick;
    to.pos_delta = from.pos_delta;
    // ENDWEAVE: The 1001 flag is the presence marker, not emptiness; a client can set a gate and
    // send an empty list.
    if (from.input_data.test(static_cast<std::size_t>(From::InputData::PERFORM_ITEM_INTERACTION))) {
        to.item_use_transaction = ew::transform(std::move(from.item_use_transaction));
    }
    if (from.input_data.test(static_cast<std::size_t>(From::InputData::PERFORM_ITEM_STACK_REQUEST))) {
        to.item_stack_request = ew::transform(std::move(from.item_stack_request));
    }
    if (from.input_data.test(static_cast<std::size_t>(From::InputData::PERFORM_BLOCK_ACTIONS))) {
        to.player_block_actions = ew::transform(std::move(from.player_block_actions));
    }
    if (from.input_data.test(static_cast<std::size_t>(From::InputData::IS_IN_CLIENT_PREDICTED_VEHICLE))) {
        to.vehicle_rot = from.vehicle_rot;
        to.client_predicted_vehicle = from.client_predicted_vehicle;
    }
    to.analog_move_vector = from.analog_move_vector;
    to.camera_orientation = from.camera_orientation;
    to.raw_move_vector = from.raw_move_vector;
    return to;
}

} // namespace endweave
