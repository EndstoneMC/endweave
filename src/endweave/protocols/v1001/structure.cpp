#include "endweave/protocols/v1001/structure.h"

#include "endweave/protocols/v1001/item_stack.h"

#include <utility>

namespace ew = endweave;

namespace endweave {

bp::StructureEditorData_<2168> Transformer<bp::StructureEditorData_<1001>>::upgrade(
    bp::StructureEditorData_<1001> &&from)
{
    bp::StructureEditorData_<2168> to;
    to.structure_name = ew::upgrade(from.structure_name);
    to.data_field = std::move(from.data_field);
    to.include_players = from.include_players;
    to.show_bounding_box = from.show_bounding_box;
    to.type = from.type;
    to.settings = std::move(from.settings);
    to.redstone_save_mode = from.redstone_save_mode;
    return to;
}

bp::StructureBlockUpdatePacket_<2168> Transformer<bp::StructureBlockUpdatePacket_<1001>>::upgrade(
    bp::StructureBlockUpdatePacket_<1001> &&from)
{
    bp::StructureBlockUpdatePacket_<2168> to;
    to.block_pos = from.block_pos;
    to.data = ew::upgrade(from.data);
    to.trigger = from.trigger;
    to.is_waterlogged = from.is_waterlogged;
    return to;
}

} // namespace endweave
