#include "movement.h"

namespace ew = endweave;

namespace endweave {
void Transformer<bp::MoveActorDeltaData_<2168>, bp::MoveActorDeltaData_<2192>>::transform(
    Context<bp::MoveActorDeltaData_<2192>> &ctx, bp::MoveActorDeltaData_<2168> &&from)
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
    to.force_completion = from.force_completion;
    // ENDWEAVE: 2168 stamps the move with no tick, so the interpolation 2192 keys on it starts from
    // zero for every actor.
    to.ticks = 0;
}

} // namespace endweave
