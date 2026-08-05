#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/dimension.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::DimensionDefinitionGroup_<2168>::DimensionDefinition,
                   bp::DimensionDefinitionGroup_<1001>::DimensionDefinition> {
    static bp::DimensionDefinitionGroup_<1001>::DimensionDefinition transform(
        bp::DimensionDefinitionGroup_<2168>::DimensionDefinition &&from);
};

template <>
struct Transformer<bp::DimensionDataPacket_<2168>, bp::DimensionDataPacket_<1001>> {
    static bp::DimensionDataPacket_<1001> transform(bp::DimensionDataPacket_<2168> &&from);
};

} // namespace endweave
