#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/dimension.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::DimensionDataPacket_<2168>::DimensionDefinition,
                   bp::DimensionDataPacket_<2193>::DimensionDefinition> {
    static void transform(Context<bp::DimensionDataPacket_<2193>::DimensionDefinition> &ctx,
                          bp::DimensionDataPacket_<2168>::DimensionDefinition &&from);
};
} // namespace endweave
