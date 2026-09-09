#include "endweave/protocols/v2168_to_v2192/movement.h"

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
