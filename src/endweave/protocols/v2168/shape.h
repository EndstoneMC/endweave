#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/shape.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::TextDataPayload_<2168>, bp::TextDataPayload_<2192>> {
    static void transform(Context<bp::TextDataPayload_<2192>> &ctx, bp::TextDataPayload_<2168> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapeDataPayload_<2168>, bp::PrimitiveShapeDataPayload_<2192>> {
    static void transform(Context<bp::PrimitiveShapeDataPayload_<2192>> &ctx,
                          bp::PrimitiveShapeDataPayload_<2168> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapesPacket_<2168>, bp::PrimitiveShapesPacket_<2192>> {
    static void transform(Context<bp::PrimitiveShapesPacket_<2192>> &ctx, bp::PrimitiveShapesPacket_<2168> &&from);
};

} // namespace endweave
