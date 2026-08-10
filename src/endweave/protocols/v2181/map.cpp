#include "endweave/protocols/v2181/map.h"

#include <bedrock/enum.hpp>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::MapDecoration_<2168> Transformer<bp::MapDecoration_<2181>, bp::MapDecoration_<2168>>::transform(
    bp::MapDecoration_<2181> &&from)
{
    using Type = bp::MapDecoration_<2168>::Type;
    bp::MapDecoration_<2168> to;
    // ENDWEAVE: the five markers 2181 added have no 2168 image, and its Count sits where AbandonedCamp
    // does, so an unknown decoration draws nothing rather than indexing off the end of the atlas.
    to.image = bp::enum_cast<Type>(bp::enum_name(from.image)).value_or(Type::NO_DRAW);
    to.rotation = from.rotation;
    to.x = from.x;
    to.y = from.y;
    to.label = std::move(from.label);
    to.color = from.color;
    return to;
}

bp::ClientboundMapItemDataPacket_<2168> Transformer<
    bp::ClientboundMapItemDataPacket_<2181>,
    bp::ClientboundMapItemDataPacket_<2168>>::transform(bp::ClientboundMapItemDataPacket_<2181> &&from)
{
    bp::ClientboundMapItemDataPacket_<2168> to;
    to.map_id = from.map_id;
    to.dimension = from.dimension;
    to.locked = from.locked;
    to.map_origin = from.map_origin;
    to.creation_map_ids = std::move(from.creation_map_ids);
    to.scale = from.scale;
    to.unique_ids = std::move(from.unique_ids);
    to.decorations = ew::transform(std::move(from.decorations));
    to.width = from.width;
    to.height = from.height;
    to.start_x = from.start_x;
    to.start_y = from.start_y;
    to.map_pixels = std::move(from.map_pixels);
    return to;
}

} // namespace endweave
