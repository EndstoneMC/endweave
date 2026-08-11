#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/map.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::MapItemTrackedActor_<2168>::UniqueId, bp::MapItemTrackedActor_<1001>::UniqueId> {
    static void transform(Context<bp::MapItemTrackedActor_<1001>::UniqueId> &ctx,
                          bp::MapItemTrackedActor_<2168>::UniqueId &&from);
};

template <>
struct Transformer<bp::MapDecoration_<2168>, bp::MapDecoration_<1001>> {
    static void transform(Context<bp::MapDecoration_<1001>> &ctx, bp::MapDecoration_<2168> &&from);
};

template <>
struct Transformer<bp::ClientboundMapItemDataPacket_<2168>, bp::ClientboundMapItemDataPacket_<1001>> {
    static void transform(Context<bp::ClientboundMapItemDataPacket_<1001>> &ctx,
                          bp::ClientboundMapItemDataPacket_<2168> &&from);
};

template <>
struct Transformer<bp::MapDecoration_<2168>, bp::MapDecoration_<2187>> {
    static void transform(Context<bp::MapDecoration_<2187>> &ctx, bp::MapDecoration_<2168> &&from);
};

template <>
struct Transformer<bp::ClientboundMapItemDataPacket_<2168>, bp::ClientboundMapItemDataPacket_<2187>> {
    static void transform(Context<bp::ClientboundMapItemDataPacket_<2187>> &ctx,
                          bp::ClientboundMapItemDataPacket_<2168> &&from);
};

} // namespace endweave
