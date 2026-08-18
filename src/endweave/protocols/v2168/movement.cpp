#include "endweave/protocols/v2168/movement.h"

#include <cstdint>
#include <utility>

namespace ew = endweave;

namespace {

// ENDWEAVE: not a Transformer -- the 2168 descriptor downgrades to two different 1001 types
// depending on whether the containing packet had cerealised yet, and one key cannot hold both.
bp::NetworkItemStackDescriptor downgradeLegacyItemStack(bp::SerializedNetworkItemStackDescriptor_<2168> &&from)
{
    bp::NetworkItemStackDescriptor to;
    to.id = from.id;
    to.stack_size = from.stack_size;
    to.aux_value = from.aux_value;
    // ENDWEAVE: 2168 codes a request id as -2n-1 and a legacy request id as -2n, and the
    // legacy descriptor holds only a bare net id, so both negative cases are dropped.
    if (from.net_id_variant.has_value() && from.net_id_variant.value() >= 0) {
        to.net_id = bp::ItemStackNetId{from.net_id_variant.value()};
    }
    // ENDWEAVE: 1001 reads the block runtime id signed and 2168 unsigned, same bits.
    to.block_runtime_id = static_cast<std::int32_t>(from.block_runtime_id);
    to.user_data_buffer = std::move(from.user_data_buffer);
    return to;
}

} // namespace

namespace endweave {

void Transformer<bp::MoveActorDeltaData_<2168>, bp::MoveActorDeltaData_<1001>>::transform(
    Context<bp::MoveActorDeltaData_<1001>> &ctx, bp::MoveActorDeltaData_<2168> &&from)
{
    auto &to = ctx.out();
    to.runtime_id = from.runtime_id;
    // ENDWEAVE: The header is rebuilt from the engaged optionals; a bit disagreeing with its
    // payload would desynchronise 1001's reader for the rest of the packet.
    to.header = 0;
    if (from.new_position_x.has_value()) {
        to.header |= MoveActorDeltaHeader::CONTAINS_POSITION_X;
        to.new_position_x = from.new_position_x.value();
    }
    if (from.new_position_y.has_value()) {
        to.header |= MoveActorDeltaHeader::CONTAINS_POSITION_Y;
        to.new_position_y = from.new_position_y.value();
    }
    if (from.new_position_z.has_value()) {
        to.header |= MoveActorDeltaHeader::CONTAINS_POSITION_Z;
        to.new_position_z = from.new_position_z.value();
    }
    if (from.rot_x.has_value()) {
        to.header |= MoveActorDeltaHeader::CONTAINS_ROTATION_X;
        to.rot_x = from.rot_x.value();
    }
    if (from.rot_y.has_value()) {
        to.header |= MoveActorDeltaHeader::CONTAINS_ROTATION_Y;
        to.rot_y = from.rot_y.value();
    }
    if (from.rot_y_head.has_value()) {
        to.header |= MoveActorDeltaHeader::CONTAINS_ROTATION_Y_HEAD;
        to.rot_y_head = from.rot_y_head.value();
    }
    if (from.is_on_ground) {
        to.header |= MoveActorDeltaHeader::IS_ON_GROUND;
    }
    if (from.force_move) {
        to.header |= MoveActorDeltaHeader::FORCE_MOVE;
    }
    if (from.force_move_local_entity) {
        to.header |= MoveActorDeltaHeader::FORCE_MOVE_LOCAL_ENTITY;
    }
    if (from.force_completion) {
        to.header |= MoveActorDeltaHeader::FORCE_COMPLETION;
    }
}

void Transformer<bp::AddPlayerPacket_<2168>, bp::AddPlayerPacket_<1001>>::transform(
    Context<bp::AddPlayerPacket_<1001>> &ctx, bp::AddPlayerPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.uuid = from.uuid;
    to.name = std::move(from.name);
    to.runtime_id = from.runtime_id;
    to.platform_online_id = std::move(from.platform_online_id);
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.rot = from.rot;
    to.y_head_rot = from.y_head_rot;
    to.carried_item = downgradeLegacyItemStack(std::move(from.carried_item));
    to.player_game_type = from.player_game_type;
    to.unpack = ew::transform(ctx, std::move(from.unpack));
    to.synched_properties = std::move(from.synched_properties);
    to.abilities_data = std::move(from.abilities_data);
    to.links = std::move(from.links);
    to.device_id = std::move(from.device_id);
    to.build_platform = from.build_platform;
}

void Transformer<bp::AddItemActorPacket_<2168>, bp::AddItemActorPacket_<1001>>::transform(
    Context<bp::AddItemActorPacket_<1001>> &ctx, bp::AddItemActorPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.id = from.id;
    to.runtime_id = from.runtime_id;
    to.item = downgradeLegacyItemStack(std::move(from.item));
    to.pos = from.pos;
    to.velocity = from.velocity;
    to.data = ew::transform(ctx, std::move(from.data));
    to.is_from_fishing = from.is_from_fishing;
}

void Transformer<bp::MovePlayerPacket_<2168>, bp::MovePlayerPacket_<1001>>::transform(
    Context<bp::MovePlayerPacket_<1001>> &ctx, bp::MovePlayerPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.player_id = from.player_id;
    to.pos = from.pos;
    to.rot = from.rot;
    to.y_head_rot = from.y_head_rot;
    to.reset_position = from.reset_position;
    to.on_ground = from.on_ground;
    to.riding_id = from.riding_id;
    // ENDWEAVE: teleport_data under a non-TELEPORT mode is dropped; forcing the mode would
    // carry it but make the client snap instead of move. TELEPORT with no data invents 0/0.
    to.cause = from.teleport_data.has_value() ? from.teleport_data.value().cause : 0;
    to.source_entity_type = from.teleport_data.has_value() ? from.teleport_data.value().source_entity_type : 0;
    to.tick = from.tick;
}

void Transformer<bp::MoveActorDeltaPacket_<2168>, bp::MoveActorDeltaPacket_<1001>>::transform(
    Context<bp::MoveActorDeltaPacket_<1001>> &ctx, bp::MoveActorDeltaPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.move_data = ew::transform(ctx, std::move(from.move_data));
}

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

void Transformer<bp::MoveActorDeltaPacket_<2168>, bp::MoveActorDeltaPacket_<2192>>::transform(
    Context<bp::MoveActorDeltaPacket_<2192>> &ctx, bp::MoveActorDeltaPacket_<2168> &&from)
{
    auto &to = ctx.out();
    to.move_data = ew::transform(ctx, std::move(from.move_data));
}

} // namespace endweave
