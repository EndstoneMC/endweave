#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MapItemTrackedActor_<1001>::UniqueId, bp::MapItemTrackedActor_<2168>::UniqueId> {
    static void transform(Context<bp::MapItemTrackedActor_<2168>::UniqueId> &ctx,
                          bp::MapItemTrackedActor_<1001>::UniqueId &&from);
};

template <>
struct Transformer<bp::MapDecoration_<1001>, bp::MapDecoration_<2168>> {
    static void transform(Context<bp::MapDecoration_<2168>> &ctx, bp::MapDecoration_<1001> &&from);
};

template <>
struct Transformer<bp::ClientboundMapItemDataPacket_<1001>, bp::ClientboundMapItemDataPacket_<2168>> {
    static void transform(Context<bp::ClientboundMapItemDataPacket_<2168>> &ctx,
                          bp::ClientboundMapItemDataPacket_<1001> &&from);
};

} // namespace endweave
