#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/shape.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::TextDataPayload_<2192>, bp::TextDataPayload_<2168>> {
    static void transform(Context<bp::TextDataPayload_<2168>> &ctx, bp::TextDataPayload_<2192> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapeDataPayload_<2192>, bp::PrimitiveShapeDataPayload_<2168>> {
    static void transform(Context<bp::PrimitiveShapeDataPayload_<2168>> &ctx,
                          bp::PrimitiveShapeDataPayload_<2192> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapesPacket_<2192>, bp::PrimitiveShapesPacket_<2168>> {
    static void transform(Context<bp::PrimitiveShapesPacket_<2168>> &ctx, bp::PrimitiveShapesPacket_<2192> &&from);
};

} // namespace endweave
