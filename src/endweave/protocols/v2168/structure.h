#pragma once

#include "endweave/protocol/transform.h"

#include <protocol/structure.h>

namespace bp = bedrock::protocol;

namespace endweave {

template <>
struct Transformer<bp::StructureEditorData_<2168>, bp::StructureEditorData_<1001>> {
    static bp::StructureEditorData_<1001> transform(bp::StructureEditorData_<2168> &&from);
};

template <>
struct Transformer<bp::StructureBlockUpdatePacket_<2168>, bp::StructureBlockUpdatePacket_<1001>> {
    static bp::StructureBlockUpdatePacket_<1001> transform(bp::StructureBlockUpdatePacket_<2168> &&from);
};

} // namespace endweave
