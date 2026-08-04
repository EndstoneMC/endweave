#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MapItemTrackedActor_<1001>::UniqueId> {
    static bp::MapItemTrackedActor_<2168>::UniqueId upgrade(bp::MapItemTrackedActor_<1001>::UniqueId &&from);
};

template <>
struct Transformer<bp::MapDecoration_<1001>> {
    static bp::MapDecoration_<2168> upgrade(bp::MapDecoration_<1001> &&from);
};

template <>
struct Transformer<bp::ClientboundMapItemDataPacket_<1001>> {
    static bp::ClientboundMapItemDataPacket_<2168> upgrade(bp::ClientboundMapItemDataPacket_<1001> &&from);
};

} // namespace endweave
