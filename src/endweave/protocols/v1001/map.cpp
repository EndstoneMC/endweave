#include "endweave/protocols/v1001/map.h"

#include <cstdint>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::MapItemTrackedActor_<2168>::UniqueId Transformer<
    bp::MapItemTrackedActor_<1001>::UniqueId,
    bp::MapItemTrackedActor_<2168>::UniqueId>::transform(bp::MapItemTrackedActor_<1001>::UniqueId &&from)
{
    using Type = bp::MapItemTrackedActor_<1001>::Type;
    bp::MapItemTrackedActor_<2168>::UniqueId to;
    to.type = static_cast<bp::MapItemTrackedActor_<2168>::Type>(from.type);
    // ENDWEAVE: 1001's type decides which id it writes, so an OTHER actor reaches 2168 with neither engaged.
    if (from.type == Type::ENTITY) {
        to.key_entity_id = from.key_entity_id;
    }
    if (from.type == Type::BLOCK_ENTITY) {
        to.key_block_pos = from.key_block_pos;
    }
    return to;
}

bp::MapDecoration_<2168> Transformer<bp::MapDecoration_<1001>, bp::MapDecoration_<2168>>::transform(
    bp::MapDecoration_<1001> &&from)
{
    bp::MapDecoration_<2168> to;
    to.image = from.image;
    to.rotation = from.rotation;
    to.x = from.x;
    to.y = from.y;
    to.label = std::move(from.label);
    to.color = from.color;
    return to;
}

bp::ClientboundMapItemDataPacket_<2168> Transformer<
    bp::ClientboundMapItemDataPacket_<1001>,
    bp::ClientboundMapItemDataPacket_<2168>>::transform(bp::ClientboundMapItemDataPacket_<1001> &&from)
{
    using Type = bp::ClientboundMapItemDataPacket_<1001>::Type;
    // ENDWEAVE: 2168 replaced 1001's bitflag word with optionals, so the bits are read here and go no further.
    const bool creation = (from.type & static_cast<std::uint32_t>(Type::CREATION)) != 0;
    const bool decoration = (from.type & static_cast<std::uint32_t>(Type::DECORATION_UPDATE)) != 0;
    const bool texture = (from.type & static_cast<std::uint32_t>(Type::TEXTURE_UPDATE)) != 0;

    bp::ClientboundMapItemDataPacket_<2168> to;
    to.map_id = from.map_id;
    to.dimension = from.dimension;
    to.locked = from.locked;
    to.map_origin = from.map_origin;
    if (creation) {
        to.map_ids = std::move(from.map_ids);
    }
    // ENDWEAVE: 1001 writes the scale for any of the three groups, so it is not a group of its own.
    if (creation || decoration || texture) {
        to.scale = from.scale;
    }
    if (decoration) {
        to.unique_ids = ew::transform(std::move(from.unique_ids));
        to.decorations = ew::transform(std::move(from.decorations));
    }
    if (texture) {
        to.width = from.width;
        to.height = from.height;
        to.start_x = from.start_x;
        to.start_y = from.start_y;
        to.map_pixels = std::move(from.map_pixels);
    }
    return to;
}

} // namespace endweave
