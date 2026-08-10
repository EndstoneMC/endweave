#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/shape.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::TextDataPayload_<2181>, bp::TextDataPayload_<2168>> {
    static bp::TextDataPayload_<2168> transform(bp::TextDataPayload_<2181> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapeDataPayload_<2181>, bp::PrimitiveShapeDataPayload_<2168>> {
    static bp::PrimitiveShapeDataPayload_<2168> transform(bp::PrimitiveShapeDataPayload_<2181> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapesPacket_<2181>, bp::PrimitiveShapesPacket_<2168>> {
    static bp::PrimitiveShapesPacket_<2168> transform(bp::PrimitiveShapesPacket_<2181> &&from);
};

} // namespace endweave
