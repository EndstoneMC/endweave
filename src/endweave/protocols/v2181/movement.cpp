#include "endweave/protocols/v2181/movement.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MoveActorDeltaData_<2181>, bp::MoveActorDeltaData_<2168>>::transform(
    Context<bp::MoveActorDeltaData_<2168>> &ctx, bp::MoveActorDeltaData_<2181> &&from)
{
    auto &to = ctx.out();
    to.runtime_id = from.runtime_id;
    to.new_position_x = from.new_position_x;
    to.new_position_y = from.new_position_y;
    to.new_position_z = from.new_position_z;
    to.rot_x = from.rot_x;
    to.rot_y = from.rot_y;
    to.rot_y_head = from.rot_y_head;
    to.is_on_ground = from.is_on_ground;
    to.force_move = from.force_move;
    to.force_move_local_entity = from.force_move_local_entity;
    // ENDWEAVE: ticks is dropped; a 2168 client times the move by its own arrival.
    to.force_completion = from.force_completion;
}

void Transformer<bp::MoveActorDeltaPacket_<2181>, bp::MoveActorDeltaPacket_<2168>>::transform(
    Context<bp::MoveActorDeltaPacket_<2168>> &ctx, bp::MoveActorDeltaPacket_<2181> &&from)
{
    auto &to = ctx.out();
    to.move_data = ew::transform(ctx, std::move(from.move_data));
}

} // namespace endweave
