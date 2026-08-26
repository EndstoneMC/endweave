#pragma once

#include "endweave/protocol/transform.h"

#include <bedrock/protocol/dimension.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DimensionDefinitionGroup_<2168>::DimensionDefinition,
                   bp::DimensionDefinitionGroup_<1001>::DimensionDefinition> {
    static void transform(Context<bp::DimensionDefinitionGroup_<1001>::DimensionDefinition> &ctx,
                          bp::DimensionDefinitionGroup_<2168>::DimensionDefinition &&from);
};

template <>
struct Transformer<bp::DimensionDefinitionGroup_<2168>::DimensionDefinition,
                   bp::DimensionDefinitionGroup_<2192>::DimensionDefinition> {
    static void transform(Context<bp::DimensionDefinitionGroup_<2192>::DimensionDefinition> &ctx,
                          bp::DimensionDefinitionGroup_<2168>::DimensionDefinition &&from);
};

} // namespace endweave
