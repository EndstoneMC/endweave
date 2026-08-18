#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/item_stack.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::ItemStackResponseSlotInfo_<2192>, bp::ItemStackResponseSlotInfo_<2168>> {
    static void transform(Context<bp::ItemStackResponseSlotInfo_<2168>> &ctx,
                          bp::ItemStackResponseSlotInfo_<2192> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseContainerInfo_<2192>, bp::ItemStackResponseContainerInfo_<2168>> {
    static void transform(Context<bp::ItemStackResponseContainerInfo_<2168>> &ctx,
                          bp::ItemStackResponseContainerInfo_<2192> &&from);
};

template <>
struct Transformer<bp::ItemStackResponseInfo_<2192>, bp::ItemStackResponseInfo_<2168>> {
    static void transform(Context<bp::ItemStackResponseInfo_<2168>> &ctx, bp::ItemStackResponseInfo_<2192> &&from);
};

template <>
struct Transformer<bp::ItemStackResponsePacket_<2192>, bp::ItemStackResponsePacket_<2168>> {
    static void transform(Context<bp::ItemStackResponsePacket_<2168>> &ctx, bp::ItemStackResponsePacket_<2192> &&from);
};

} // namespace endweave
