#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/dimension.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DimensionDefinitionGroup_<2192>::DimensionDefinition,
                   bp::DimensionDefinitionGroup_<2168>::DimensionDefinition> {
    static void transform(Context<bp::DimensionDefinitionGroup_<2168>::DimensionDefinition> &ctx,
                          bp::DimensionDefinitionGroup_<2192>::DimensionDefinition &&from);
};

} // namespace endweave
