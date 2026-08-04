#include "endweave/protocols/v2168/input.h"

#include <cstddef>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::PlayerBlockActionData_<1001> Transformer<bp::PlayerBlockActionData_<2168>>::downgrade(
    bp::PlayerBlockActionData_<2168> &&from)
{
    bp::PlayerBlockActionData_<1001> to;
    // ENDWEAVE: TODO INTERNAL_UPDATE collides with 1001's COUNT sentinel, so it maps to UNKNOWN,
    // which BDS ignores. Dropping the action would be truer, but a transform is one-to-one.
    to.player_action_type = from.player_action_type == bp::PlayerActionType_<2168>::INTERNAL_UPDATE
                              ? bp::PlayerActionType_<1001>::UNKNOWN
                              : static_cast<bp::PlayerActionType_<1001>>(from.player_action_type);
    to.pos = from.pos;
    to.facing = from.facing;
    return to;
}

bp::PlayerAuthInputPacket_<1001> Transformer<bp::PlayerAuthInputPacket_<2168>>::downgrade(
    bp::PlayerAuthInputPacket_<2168> &&from)
{
    using To = bp::PlayerAuthInputPacket_<1001>;

    bp::PlayerAuthInputPacket_<1001> to;
    to.rot = from.rot;
    to.pos = from.pos;
    to.move = from.move;
    to.y_head_rot = from.y_head_rot;
    // ENDWEAVE: INTERNAL_UPDATE is bit 65, past the end of 1001's bitset, so it is dropped.
    for (const auto flag : from.input_data) {
        const auto bit = static_cast<std::size_t>(flag);
        if (bit < to.input_data.size()) {
            to.input_data.set(bit);
        }
    }
    // ENDWEAVE: The gate flags are recomputed from the engaged payloads; at 1001 the flag drives
    // the reader, and one that disagrees eats the rest of the packet.
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
    // ENDWEAVE: TODO item_use_transaction is dropped -- 1001 has no field for it -- so a client's
    // block placements and item uses never reach the server.
    if (from.item_stack_request.has_value()) {
        to.item_stack_request = ew::downgrade(from.item_stack_request.value());
    }
    if (from.player_block_actions.has_value()) {
        to.player_block_actions = ew::downgrade(from.player_block_actions.value());
    }
    // ENDWEAVE: 1001 cannot say "absent" here, but the gate flag is clear, so the zeroes never
    // reach the wire.
    to.vehicle_rot = from.vehicle_rot.value_or(bp::Vec2{});
    to.client_predicted_vehicle = from.client_predicted_vehicle.value_or(bp::ActorUniqueID{});
    to.analog_move_vector = from.analog_move_vector;
    to.camera_orientation = from.camera_orientation;
    to.raw_move_vector = from.raw_move_vector;
    return to;
}

} // namespace endweave
