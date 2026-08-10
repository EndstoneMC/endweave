#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/shape.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::TextDataPayload_<2181>, bp::TextDataPayload_<2168>> {
    static void transform(Context<bp::TextDataPayload_<2168>> &ctx, bp::TextDataPayload_<2181> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapeDataPayload_<2181>, bp::PrimitiveShapeDataPayload_<2168>> {
    static void transform(Context<bp::PrimitiveShapeDataPayload_<2168>> &ctx,
                          bp::PrimitiveShapeDataPayload_<2181> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapesPacket_<2181>, bp::PrimitiveShapesPacket_<2168>> {
    static void transform(Context<bp::PrimitiveShapesPacket_<2168>> &ctx, bp::PrimitiveShapesPacket_<2181> &&from);
};

} // namespace endweave
