#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/dimension.h>

namespace bp = bedrock::protocol;

namespace endweave {
template <>
struct Transformer<bp::DimensionDefinitionGroup_<2193>::DimensionDefinition,
                   bp::DimensionDefinitionGroup_<2225>::DimensionDefinition> {
    static void transform(Context<bp::DimensionDefinitionGroup_<2225>::DimensionDefinition> &ctx,
                          bp::DimensionDefinitionGroup_<2193>::DimensionDefinition &&from);
};
} // namespace endweave
