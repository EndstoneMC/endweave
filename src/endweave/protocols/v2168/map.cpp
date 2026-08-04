#include "endweave/protocols/v2168/map.h"

#include <cstdint>
#include <utility>

namespace ew = endweave;

namespace endweave {

bp::MapItemTrackedActor_<1001>::UniqueId Transformer<bp::MapItemTrackedActor_<2168>::UniqueId>::downgrade(
    bp::MapItemTrackedActor_<2168>::UniqueId &&from)
{
    bp::MapItemTrackedActor_<1001>::UniqueId to;
    to.type = static_cast<bp::MapItemTrackedActor_<1001>::Type>(from.type);
    // ENDWEAVE: 1001 writes only the id its type names, so the other never reaches the wire.
    to.key_entity_id = from.key_entity_id.value_or(bp::ActorUniqueID{});
    to.key_block_pos = from.key_block_pos.value_or(bp::BlockPos{});
    return to;
}

bp::MapDecoration_<1001> Transformer<bp::MapDecoration_<2168>>::downgrade(bp::MapDecoration_<2168> &&from)
{
    bp::MapDecoration_<1001> to;
    to.image = from.image;
    to.rotation = from.rotation;
    to.x = from.x;
    to.y = from.y;
    to.label = std::move(from.label);
    to.color = from.color;
    return to;
}

bp::ClientboundMapItemDataPacket_<1001> Transformer<bp::ClientboundMapItemDataPacket_<2168>>::downgrade(
    bp::ClientboundMapItemDataPacket_<2168> &&from)
{
    using Type = bp::ClientboundMapItemDataPacket_<1001>::Type;
    // ENDWEAVE: 1001's flag word is rebuilt from which optionals arrived, one bit per group with any field engaged.
    const bool decoration = from.unique_ids.has_value() || from.decorations.has_value();
    const bool texture = from.width.has_value() || from.height.has_value() || from.start_x.has_value() ||
                         from.start_y.has_value() || from.map_pixels.has_value();

    bp::ClientboundMapItemDataPacket_<1001> to;
    to.map_id = from.map_id;
    to.type = (from.map_ids.has_value() ? static_cast<std::uint32_t>(Type::CREATION) : 0) |
              (decoration ? static_cast<std::uint32_t>(Type::DECORATION_UPDATE) : 0) |
              (texture ? static_cast<std::uint32_t>(Type::TEXTURE_UPDATE) : 0);
    to.dimension = from.dimension;
    to.locked = from.locked;
    to.map_origin = from.map_origin;
    if (from.map_ids.has_value()) {
        to.map_ids = std::move(from.map_ids).value();
    }
    // ENDWEAVE: 1001 has no bit for the scale alone, so a scale sent without any of the three groups is lost.
    to.scale = from.scale.value_or(0);
    if (from.unique_ids.has_value()) {
        to.unique_ids = ew::downgrade(from.unique_ids.value());
    }
    if (from.decorations.has_value()) {
        to.decorations = ew::downgrade(from.decorations.value());
    }
    // ENDWEAVE: 1001 writes the five texture fields together, so one present drags the other four out at zero.
    to.width = from.width.value_or(0);
    to.height = from.height.value_or(0);
    to.start_x = from.start_x.value_or(0);
    to.start_y = from.start_y.value_or(0);
    if (from.map_pixels.has_value()) {
        to.map_pixels = std::move(from.map_pixels).value();
    }
    return to;
}

} // namespace endweave
