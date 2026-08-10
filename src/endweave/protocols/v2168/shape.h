#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/shape.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::TextDataPayload_<2168>, bp::TextDataPayload_<2181>> {
    static bp::TextDataPayload_<2181> transform(bp::TextDataPayload_<2168> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapeDataPayload_<2168>, bp::PrimitiveShapeDataPayload_<2181>> {
    static bp::PrimitiveShapeDataPayload_<2181> transform(bp::PrimitiveShapeDataPayload_<2168> &&from);
};

template <>
struct Transformer<bp::PrimitiveShapesPacket_<2168>, bp::PrimitiveShapesPacket_<2181>> {
    static bp::PrimitiveShapesPacket_<2181> transform(bp::PrimitiveShapesPacket_<2168> &&from);
};

} // namespace endweave
