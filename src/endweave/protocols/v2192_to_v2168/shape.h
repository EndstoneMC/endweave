#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/script.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::PrimitiveShapeDataPayload_<2192>, bp::PrimitiveShapeDataPayload_<2168>> {
    static void transform(Context<bp::PrimitiveShapeDataPayload_<2168>> &ctx,
                          bp::PrimitiveShapeDataPayload_<2192> &&from);
};

} // namespace endweave
