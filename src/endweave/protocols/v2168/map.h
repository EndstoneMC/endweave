#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MapItemTrackedActor_<2168>::UniqueId> {
    static bp::MapItemTrackedActor_<1001>::UniqueId downgrade(bp::MapItemTrackedActor_<2168>::UniqueId &&from);
};

template <>
struct Transformer<bp::MapDecoration_<2168>> {
    static bp::MapDecoration_<1001> downgrade(bp::MapDecoration_<2168> &&from);
};

template <>
struct Transformer<bp::ClientboundMapItemDataPacket_<2168>> {
    static bp::ClientboundMapItemDataPacket_<1001> downgrade(bp::ClientboundMapItemDataPacket_<2168> &&from);
};

} // namespace endweave
