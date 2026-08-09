#include "endweave/protocols/v1001/movement.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

bp::MoveActorDeltaData_<2168> Transformer<bp::MoveActorDeltaData_<1001>, bp::MoveActorDeltaData_<2168>>::transform(
    bp::MoveActorDeltaData_<1001> &&from)
{
    bp::MoveActorDeltaData_<2168> to;
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
    return to;
}

bp::AddPlayerPacket_<2168> Transformer<bp::AddPlayerPacket_<1001>, bp::AddPlayerPacket_<2168>>::transform(
    bp::AddPlayerPacket_<1001> &&from)
{
    bp::AddPlayerPacket_<2168> to;
    to.uuid = from.uuid;
    to.name = std::move(from.name);
    to.runtime_id = from.runtime_id;
    to.platform_online_id = std::move(from.platform_online_id);
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.rot = from.rot;
    to.y_head_rot = from.y_head_rot;
    to.carried_item = ew::transform(std::move(from.carried_item));
    to.player_game_type = from.player_game_type;
    to.unpack = ew::transform(std::move(from.unpack));
    to.synched_properties = std::move(from.synched_properties);
    to.abilities_data = std::move(from.abilities_data);
    to.links = std::move(from.links);
    to.device_id = std::move(from.device_id);
    to.build_platform = from.build_platform;
    return to;
}

bp::AddItemActorPacket_<2168> Transformer<bp::AddItemActorPacket_<1001>, bp::AddItemActorPacket_<2168>>::transform(
    bp::AddItemActorPacket_<1001> &&from)
{
    bp::AddItemActorPacket_<2168> to;
    to.id = from.id;
    to.runtime_id = from.runtime_id;
    to.item = ew::transform(std::move(from.item));
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.data = ew::transform(std::move(from.data));
    to.is_from_fishing = from.is_from_fishing;
    return to;
}

bp::MovePlayerPacket_<2168> Transformer<bp::MovePlayerPacket_<1001>, bp::MovePlayerPacket_<2168>>::transform(
    bp::MovePlayerPacket_<1001> &&from)
{
    bp::MovePlayerPacket_<2168> to;
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
    return to;
}

bp::MoveActorDeltaPacket_<2168> Transformer<
    bp::MoveActorDeltaPacket_<1001>, bp::MoveActorDeltaPacket_<2168>>::transform(bp::MoveActorDeltaPacket_<1001> &&from)
{
    bp::MoveActorDeltaPacket_<2168> to;
    to.move_data = ew::transform(std::move(from.move_data));
    return to;
}

} // namespace endweave
