#include "endweave/protocols/v1001/movement.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::MoveActorDeltaData_<1001>, bp::MoveActorDeltaData_<2168>>::transform(
    Context<bp::MoveActorDeltaData_<2168>> &ctx, bp::MoveActorDeltaData_<1001> &&from)
{
    auto &to = ctx.out();
    to.runtime_id = from.runtime_id;
    // ENDWEAVE: A clear header bit means the component was never on the wire, so it becomes
    // nullopt rather than a zero.
    if ((from.header & MoveActorDeltaHeader::CONTAINS_POSITION_X) != 0) {
        to.new_position_x = from.new_position_x;
    }
    if ((from.header & MoveActorDeltaHeader::CONTAINS_POSITION_Y) != 0) {
        to.new_position_y = from.new_position_y;
    }
    if ((from.header & MoveActorDeltaHeader::CONTAINS_POSITION_Z) != 0) {
        to.new_position_z = from.new_position_z;
    }
    if ((from.header & MoveActorDeltaHeader::CONTAINS_ROTATION_X) != 0) {
        to.rot_x = from.rot_x;
    }
    if ((from.header & MoveActorDeltaHeader::CONTAINS_ROTATION_Y) != 0) {
        to.rot_y = from.rot_y;
    }
    if ((from.header & MoveActorDeltaHeader::CONTAINS_ROTATION_Y_HEAD) != 0) {
        to.rot_y_head = from.rot_y_head;
    }
    to.is_on_ground = (from.header & MoveActorDeltaHeader::IS_ON_GROUND) != 0;
    to.force_move = (from.header & MoveActorDeltaHeader::FORCE_MOVE) != 0;
    to.force_move_local_entity = (from.header & MoveActorDeltaHeader::FORCE_MOVE_LOCAL_ENTITY) != 0;
    to.force_completion = (from.header & MoveActorDeltaHeader::FORCE_COMPLETION) != 0;
}

void Transformer<bp::MovePlayerPacket_<1001>, bp::MovePlayerPacket_<2168>>::transform(
    Context<bp::MovePlayerPacket_<2168>> &ctx, bp::MovePlayerPacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.player_id = from.player_id;
    to.pos = from.pos;
    to.rot = from.rot;
    to.y_head_rot = from.y_head_rot;
    to.reset_position = from.reset_position;
    to.on_ground = from.on_ground;
    to.riding_id = from.riding_id;
    // ENDWEAVE: 1001 writes the pair only under TELEPORT, so the mode is the presence marker 2168
    // makes explicit.
    if (from.reset_position == bp::PlayerPositionModeComponent::PositionMode::TELEPORT) {
        to.teleport_data = bp::MovePlayerTeleportData_<2168>{from.cause, from.source_entity_type};
    }
    to.tick = from.tick;
}

} // namespace endweave
