#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MapDecoration_<2181>, bp::MapDecoration_<2168>> {
    static bp::MapDecoration_<2168> transform(bp::MapDecoration_<2181> &&from);
};

template <>
struct Transformer<bp::ClientboundMapItemDataPacket_<2181>, bp::ClientboundMapItemDataPacket_<2168>> {
    static bp::ClientboundMapItemDataPacket_<2168> transform(bp::ClientboundMapItemDataPacket_<2181> &&from);
};

} // namespace endweave
