#include "endweave/protocols/v1001/structure.h"

#include "endweave/protocols/v1001/item_stack.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

void Transformer<bp::StructureEditorData_<1001>, bp::StructureEditorData_<2168>>::transform(
    Context<bp::StructureEditorData_<2168>> &ctx, bp::StructureEditorData_<1001> &&from)
{
    auto &to = ctx.out();
    to.structure_name = ew::transform(ctx, std::move(from.structure_name));
    to.data_field = std::move(from.data_field);
    to.include_players = from.include_players;
    to.show_bounding_box = from.show_bounding_box;
    to.type = from.type;
    to.settings = std::move(from.settings);
    to.redstone_save_mode = from.redstone_save_mode;
}

void Transformer<bp::StructureBlockUpdatePacket_<1001>, bp::StructureBlockUpdatePacket_<2168>>::transform(
    Context<bp::StructureBlockUpdatePacket_<2168>> &ctx, bp::StructureBlockUpdatePacket_<1001> &&from)
{
    auto &to = ctx.out();
    to.block_pos = from.block_pos;
    to.data = ew::transform(ctx, std::move(from.data));
    to.trigger = from.trigger;
    to.is_waterlogged = from.is_waterlogged;
}

} // namespace endweave
